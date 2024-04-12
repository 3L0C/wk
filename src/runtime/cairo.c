#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

/* Cairo and pango includes */
#include <cairo.h>
#include <pango/pango-font.h>
#include <pango/pango-layout.h>
#include <pango/pango-types.h>
#include <pango/pangocairo.h>

/* common includes */
#include "common/common.h"
#include "common/debug.h"
#include "common/menu.h"
#include "common/types.h"
#include "common/util.h"

/* local includes */
#include "cairo.h"

static Cairo* cairo;
static WkMenu* mainMenu;
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
cairoGetHeight(WkMenu* menu, cairo_surface_t* surface, uint32_t maxHeight)
{
    assert(menu && surface);

    uint32_t height = 0;
    countMenuKeyChords(menu);

    cairo_t* cr = cairo_create(surface);
    PangoLayout* layout = pango_cairo_create_layout(cr);
    PangoFontDescription* fontDesc = pango_font_description_from_string(menu->font);
    PangoRectangle rect;

    calculateGrid(menu->keyChordCount, menu->maxCols, &menu->rows, &menu->cols);

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

    menu->cellHeight = (rect.height + menu->hpadding * 2);
    height = menu->cellHeight * menu->rows + (menu->borderWidth * 2);
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
cairoInitPaint(WkMenu* menu, CairoPaint* paint)
{
    cairoSetColors(paint, menu->colors);
    paint->font = menu->font;
}

static bool
setSourceRgba(WkColor type)
{
    CairoColor* color = NULL;

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

static void
cairoDrawRoundedPath(double radius)
{
    cairo_t* cr = cairo->cr;
    double degrees = M_PI / 180;
    double x = mainMenu->borderWidth / 2.0;
    double y = mainMenu->borderWidth / 2.0;
    double w = width - mainMenu->borderWidth;
    double h = height - mainMenu->borderWidth;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - radius, y + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc(cr, x + w - radius, y + h - radius, radius, 0 * degrees, 90 * degrees);
    cairo_arc(cr, x + radius, y + h - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);
}

static bool
drawBackground()
{
    assert(cairo && mainMenu);

    if (!setSourceRgba(WK_COLOR_BACKGROUND)) return false;

    double radius = mainMenu->borderRadius;

    if (!radius) {
        cairo_paint(cairo->cr);
    } else {
        cairoDrawRoundedPath(radius);
        cairo_fill(cairo->cr);
    }

    return true;
}

static bool
drawBorder()
{
    assert(cairo && mainMenu);

    double lineW = cairo_get_line_width(cairo->cr);
    cairo_set_line_width(cairo->cr, mainMenu->borderWidth);
    if (!setSourceRgba(WK_COLOR_BORDER)) return false;

    double radius = mainMenu->borderRadius;

    if (!radius) {
        double x = mainMenu->borderWidth / 2.0;
        double y = mainMenu->borderWidth / 2.0;
        double w = width - mainMenu->borderWidth;
        double h = height - mainMenu->borderWidth;
        cairo_rectangle(cairo->cr, x, y, w, h);
    } else {
        cairoDrawRoundedPath(radius);
    }

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
    assert(cairo && mainMenu);

    if (mainMenu->borderWidth * 2 >= width)
    {
        errorMsg("Border is larger than windo width.");
        goto end;
    }

    uint32_t startx = mainMenu->borderWidth;
    uint32_t starty = mainMenu->borderWidth;
    uint32_t rows = mainMenu->rows;
    uint32_t cols = mainMenu->cols;
    uint32_t wpadding = mainMenu->wpadding;
    uint32_t hpadding = mainMenu->hpadding;
    uint32_t cellWidth = (width - (mainMenu->borderWidth * 2)) / cols;
    uint32_t cellHeight = mainMenu->cellHeight;
    uint32_t idx = 0;
    uint32_t count = mainMenu->keyChordCount;
    PangoLayout* layout = pango_cairo_create_layout(cairo->cr);
    PangoFontDescription* fontDesc = pango_font_description_from_string(mainMenu->font);

    pango_layout_set_font_description(layout, fontDesc);
    pango_font_description_free(fontDesc);

    if (!setSourceRgba(WK_COLOR_FOREGROUND)) goto fail;

    if (mainMenu->debug)
    {
        debugMenu(mainMenu);
        debugGrid(startx, starty, rows, cols, wpadding, hpadding, cellWidth, cellHeight, count);
        debugKeyChordsShallow(mainMenu->keyChords, mainMenu->keyChordCount);
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
            drawHintText(layout, mainMenu->keyChords[idx].hint, cellWidth - (wpadding * 2));
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
cairoPaint(Cairo* cr, WkMenu* menu)
{
    assert(cr && menu);

    cairo = cr;
    mainMenu = menu;
    width = menu->width;
    height = menu->height;

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
