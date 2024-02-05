#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <cairo.h>
#include <pango/pango-font.h>
#include <pango/pango-layout.h>
#include <pango/pango-types.h>
#include <pango/pangocairo.h>

#include "cairo.h"
#include "common.h"
#include "debug.h"
#include "properties.h"
#include "types.h"
#include "util.h"
#include "window.h"


static Cairo* cairo;
static WkProperties* properties;
static uint32_t width;
static uint32_t height;
static int ellipsisWidth = -1;
static int ellipsisHeight = -1;

bool
cairoCreateForSurface(Cairo* cairo, cairo_surface_t* surface)
{
    assert(cairo && surface);

    cairo->cr = cairo_create(surface);
    if (!cairo->cr) goto fail;

    cairo->surface = surface;
    assert(cairo->scale > 0);
    cairo_surface_set_device_scale(surface, cairo->scale, cairo->scale);
    return true;

fail:
    if (cairo->cr) cairo_destroy(cairo->cr);
    return false;
}

void
cairoDestroy(Cairo* cairo)
{
    if (cairo->cr) cairo_destroy(cairo->cr);
    if (cairo->surface) cairo_surface_destroy(cairo->surface);
}

uint32_t
cairoGetHeight(WkProperties* props, cairo_surface_t* surface, uint32_t maxHeight)
{
    assert(props && surface);

    uint32_t height = 0;
    countChords(props);

    cairo_t* cr = cairo_create(surface);
    PangoLayout* layout = pango_cairo_create_layout(cr);
    PangoFontDescription* fontDesc = pango_font_description_from_string(props->font);
    PangoRectangle rect;

    calculateGrid(props->chordCount, props->maxCols, &props->rows, &props->cols);

    pango_layout_set_text(
        layout,
        "!\"#$%%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~",
        -1
    );
    pango_layout_set_font_description(layout, fontDesc);
    pango_layout_set_single_paragraph_mode(layout, 1);
    pango_layout_get_pixel_extents(layout, NULL, &rect);

    /* cleanup */
    pango_font_description_free(fontDesc);
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    props->cellHeight = (rect.height + props->hpadding * 2);
    height = props->cellHeight * props->rows + (props->borderWidth * 2);
    return height > maxHeight ? maxHeight : height;
}

static void
cairoSetColors(CairoPaint* paint, WkHexColor* colors)
{
    /* foreground */
    paint->fg.r = (float)colors[WK_COLOR_FOREGROUND].r / 255.0f;
    paint->fg.g = (float)colors[WK_COLOR_FOREGROUND].g / 255.0f;
    paint->fg.b = (float)colors[WK_COLOR_FOREGROUND].b / 255.0f;
    paint->fg.a = (float)colors[WK_COLOR_FOREGROUND].a / 255.0f;

    /* background */
    paint->bg.r = (float)colors[WK_COLOR_BACKGROUND].r / 255.0f;
    paint->bg.g = (float)colors[WK_COLOR_BACKGROUND].g / 255.0f;
    paint->bg.b = (float)colors[WK_COLOR_BACKGROUND].b / 255.0f;
    paint->bg.a = (float)colors[WK_COLOR_BACKGROUND].a / 255.0f;

    /* border */
    paint->bd.r = (float)colors[WK_COLOR_BORDER].r / 255.0f;
    paint->bd.g = (float)colors[WK_COLOR_BORDER].g / 255.0f;
    paint->bd.b = (float)colors[WK_COLOR_BORDER].b / 255.0f;
    paint->bd.a = (float)colors[WK_COLOR_BORDER].a / 255.0f;
}

void
cairoInitPaint(WkProperties* props, CairoPaint* paint)
{
    cairoSetColors(paint, props->colors);
    paint->font = props->font;
}

static bool
setSourceRgba(WkColor type)
{
    CairoColor* color;

    switch (type)
    {
    case WK_COLOR_FOREGROUND: color = &cairo->paint->fg; break;
    case WK_COLOR_BACKGROUND: color = &cairo->paint->bg; break;
    case WK_COLOR_BORDER: color = &cairo->paint->bd; break;
    default: errorMsg("Invalid color request %d", type); return false;
    }

    cairo_set_source_rgba(cairo->cr, color->r, color->g, color->b, color->a);
    return true;
}

static bool
drawBackground()
{
    assert(cairo && properties);

    if (!setSourceRgba(WK_COLOR_BACKGROUND)) return false;

    cairo_paint(cairo->cr);

    return true;
}

static bool
drawBorder()
{
    assert(cairo && properties);

    double lineW = cairo_get_line_width(cairo->cr);
    cairo_set_line_width(cairo->cr, properties->borderWidth);
    if (!setSourceRgba(WK_COLOR_BORDER)) return false;
    cairo_rectangle(cairo->cr, 0, 0, width, height);
    cairo_stroke(cairo->cr);
    cairo_set_line_width(cairo->cr, lineW);
    return true;
}

static void
drawTruncatedHintText(PangoLayout* layout, const char* text, uint32_t cellw)
{
    size_t len = strlen(text);
    size_t beglen = len;
    int textw, texth;
    char buffer[len + 1]; /* +1 for null byte '\0' */
    memcpy(buffer, text, len + 1); /* +1 to copy null byte '\0' */

    pango_layout_set_text(layout, buffer, len);
    pango_layout_get_pixel_size(layout, &textw, &texth);

    while (len && (uint32_t)(textw + ellipsisWidth) > cellw)
    {
        len--;
        while (len && !isUtf8StartByte(buffer[len])) len--;
        pango_layout_set_text(layout, buffer, len);
        pango_layout_get_pixel_size(layout, &textw, &texth);
    }

    if (len < beglen && ellipsisWidth)
    {
        memcpy(buffer + len, "...", 4);
        pango_layout_set_text(layout, buffer, -1);
    }
}

static void
drawHintText(PangoLayout* layout, const char* text, uint32_t cellw)
{
    assert(layout && text);

    int w, h;
    pango_layout_set_text(layout, text, -1);
    pango_layout_get_pixel_size(layout, &w, &h);

    if ((uint32_t)w > cellw)
    {
        drawTruncatedHintText(layout, text, cellw);
    }
}

static bool
drawGrid()
{
    assert(cairo && properties);

    if (properties->borderWidth * 2 >= width)
    {
        errorMsg("Border is larger than windo width.");
        goto end;
    }

    uint32_t startx = properties->borderWidth;
    uint32_t starty = properties->borderWidth;
    uint32_t rows = properties->rows;
    uint32_t cols = properties->cols;
    uint32_t wpadding = properties->wpadding;
    uint32_t hpadding = properties->hpadding;
    uint32_t cellWidth = (width - (properties->borderWidth * 2)) / cols;
    uint32_t cellHeight = properties->cellHeight;
    uint32_t idx = 0;
    uint32_t count = properties->chordCount;
    PangoLayout* layout = pango_cairo_create_layout(cairo->cr);
    PangoFontDescription* fontDesc = pango_font_description_from_string(properties->font);

    pango_layout_set_font_description(layout, fontDesc);
    pango_font_description_free(fontDesc);

    if (!setSourceRgba(WK_COLOR_FOREGROUND)) goto fail;

    if (properties->debug)
    {
        debugProperties(properties);
        debugChordsShallow(properties->chords, properties->chordCount);
        debugGrid(startx, starty, rows, cols, wpadding, hpadding, cellWidth, cellHeight, count);
    }

    if (ellipsisWidth == -1 || ellipsisHeight == -1)
    {
        pango_layout_set_text(layout, "...", -1);
        pango_layout_get_pixel_size(layout, &ellipsisWidth, &ellipsisHeight);
    }

    if ((wpadding * 2) >= cellWidth)
    {
        errorMsg("Width padding is larger than cell size. Unable to draw anything.");
        goto fail;
    }

    if ((uint32_t)ellipsisWidth > cellWidth - (wpadding * 2))
    {
        warnMsg("Not enough cell space to draw truncated hints.");
        ellipsisWidth = 0;
        ellipsisHeight = -1;
    }

    for (uint32_t i = 0; i < cols && idx < count; i++)
    {
        uint32_t x = startx + (i * cellWidth) + wpadding;
        for (uint32_t j = 0; j < rows && idx < count; j++, idx++)
        {
            uint32_t y = starty + (j * cellHeight) + hpadding;
            drawHintText(layout, properties->chords[idx].hint, cellWidth - (wpadding * 2));
            cairo_move_to(cairo->cr, x, y);
            pango_cairo_show_layout(cairo->cr, layout);
        }
    }

    g_object_unref(layout);
    return true;

fail:
    g_object_unref(layout);
end:
    return false;
}

void
cairoPaint(Cairo* cr, WkProperties* props)
{
    assert(cr && props);

    cairo = cr;
    properties = props;
    width = props->width;
    height = props->height;

    height /= cairo->scale;

    if (!drawBackground())
    {
        errorMsg("Could not draw background.");
        goto fail;
    }

    if (!drawBorder())
    {
        errorMsg("Could not draw border.");
        goto fail;
    }

    if (!drawGrid())
    {
        errorMsg("Could not draw grid.");
        goto fail;
    }

fail:
    return;
}
