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
#include "common/key_chord.h"

/* local includes */
#include "cairo.h"

static Cairo* cairo;
static Menu* mainMenu;
static uint32_t width;
static uint32_t height;
static int ellipsisWidth = -1;
static int ellipsisHeight = -1;
static bool ellipsisIsSet = false;

bool
cairoCreateForSurface(Cairo* cairo, cairo_surface_t* surface)
{
    assert(cairo), assert(surface);

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
    assert(cairo);

    if (cairo->cr) cairo_destroy(cairo->cr);
    if (cairo->surface) cairo_surface_destroy(cairo->surface);
}

static void
calculateGrid(const uint32_t count, const uint32_t maxCols, uint32_t* rows, uint32_t* cols)
{
    assert(rows), assert(cols);

    if (maxCols == 0 || maxCols >= count)
    {
        *rows = 1;
        *cols = count;
    }
    else
    {
        *rows = (count + maxCols - 1) / maxCols;
        *cols = (count + *rows - 1) / *rows;
    }
}

uint32_t
cairoGetHeight(Menu* menu, cairo_surface_t* surface, uint32_t maxHeight)
{
    assert(menu), assert(surface);

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
cairoSetColors(CairoPaint* paint, MenuHexColor* colors)
{
    assert(paint), assert(colors);

    /* foreground - key */
    paint->fgKey.r = (float)colors[MENU_COLOR_KEY].r / 255.0f;
    paint->fgKey.g = (float)colors[MENU_COLOR_KEY].g / 255.0f;
    paint->fgKey.b = (float)colors[MENU_COLOR_KEY].b / 255.0f;
    paint->fgKey.a = (float)colors[MENU_COLOR_KEY].a / 255.0f;

    /* foreground - delimiter */
    paint->fgDelimiter.r = (float)colors[MENU_COLOR_DELIMITER].r / 255.0f;
    paint->fgDelimiter.g = (float)colors[MENU_COLOR_DELIMITER].g / 255.0f;
    paint->fgDelimiter.b = (float)colors[MENU_COLOR_DELIMITER].b / 255.0f;
    paint->fgDelimiter.a = (float)colors[MENU_COLOR_DELIMITER].a / 255.0f;

    /* foreground - prefix */
    paint->fgPrefix.r = (float)colors[MENU_COLOR_PREFIX].r / 255.0f;
    paint->fgPrefix.g = (float)colors[MENU_COLOR_PREFIX].g / 255.0f;
    paint->fgPrefix.b = (float)colors[MENU_COLOR_PREFIX].b / 255.0f;
    paint->fgPrefix.a = (float)colors[MENU_COLOR_PREFIX].a / 255.0f;

    /* foreground - chord */
    paint->fgChord.r = (float)colors[MENU_COLOR_CHORD].r / 255.0f;
    paint->fgChord.g = (float)colors[MENU_COLOR_CHORD].g / 255.0f;
    paint->fgChord.b = (float)colors[MENU_COLOR_CHORD].b / 255.0f;
    paint->fgChord.a = (float)colors[MENU_COLOR_CHORD].a / 255.0f;

    /* background */
    paint->bg.r = (float)colors[MENU_COLOR_BACKGROUND].r / 255.0f;
    paint->bg.g = (float)colors[MENU_COLOR_BACKGROUND].g / 255.0f;
    paint->bg.b = (float)colors[MENU_COLOR_BACKGROUND].b / 255.0f;
    paint->bg.a = (float)colors[MENU_COLOR_BACKGROUND].a / 255.0f;

    /* border */
    paint->bd.r = (float)colors[MENU_COLOR_BORDER].r / 255.0f;
    paint->bd.g = (float)colors[MENU_COLOR_BORDER].g / 255.0f;
    paint->bd.b = (float)colors[MENU_COLOR_BORDER].b / 255.0f;
    paint->bd.a = (float)colors[MENU_COLOR_BORDER].a / 255.0f;
}

void
cairoInitPaint(Menu* menu, CairoPaint* paint)
{
    assert(menu), assert(paint);

    cairoSetColors(paint, menu->colors);
    paint->font = menu->font;
}

static bool
setSourceRgba(MenuColor type)
{
    CairoColor* color = NULL;

    switch (type)
    {
    case MENU_COLOR_KEY: color = &cairo->paint->fgKey; break;
    case MENU_COLOR_DELIMITER: color = &cairo->paint->fgDelimiter; break;
    case MENU_COLOR_PREFIX: color = &cairo->paint->fgPrefix; break;
    case MENU_COLOR_CHORD: color = &cairo->paint->fgChord; break;
    case MENU_COLOR_BACKGROUND: color = &cairo->paint->bg; break;
    case MENU_COLOR_BORDER: color = &cairo->paint->bd; break;
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
    assert(cairo), assert(mainMenu);

    if (!setSourceRgba(MENU_COLOR_BACKGROUND)) return false;

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
    assert(cairo), assert(mainMenu);

    double lineW = cairo_get_line_width(cairo->cr);
    cairo_set_line_width(cairo->cr, mainMenu->borderWidth);
    if (!setSourceRgba(MENU_COLOR_BORDER)) return false;

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
drawTruncatedText(PangoLayout* layout, const char* text, uint32_t cellw)
{
    assert(layout), assert(text);
    if ((uint32_t)ellipsisWidth > cellw) return;

    size_t len = strlen(text);
    int textw;
    int texth;
    uint32_t truncatedWidth = 0;
    char buffer[len + 1]; /* +1 for null byte '\0' */
    memcpy(buffer, text, len + 1); /* +1 to copy null byte '\0' */

    pango_layout_set_text(layout, buffer, len);
    pango_layout_get_pixel_size(layout, &textw, &texth);

    truncatedWidth = textw + ellipsisWidth;
    if (truncatedWidth <= cellw) return;

    size_t left = 0;
    size_t right = len;
    while (left < right)
    {
        while (left < right && isUtf8ContByte(buffer[left])) left++;
        size_t mid = (left + right) / 2;
        while (mid > left && !isUtf8StartByte(buffer[mid])) mid--;

        pango_layout_set_text(layout, buffer, mid);
        pango_layout_get_pixel_size(layout, &textw, &texth);

        truncatedWidth = textw + ellipsisWidth;
        if (truncatedWidth < cellw)
        {
            left = mid + 1;
        }
        else if (truncatedWidth == cellw)
        {
            left = mid;
            break;
        }
        else
        {
            right = mid;
        }
    }

    len = truncatedWidth == cellw ? left : left - 1;
    if (truncatedWidth != cellw && len)
    {
        while (len && !isUtf8StartByte(buffer[len])) len--;
    }

    memcpy(buffer + len, "...", 4);
    pango_layout_set_text(layout, buffer, -1);
}

static bool
drawText(PangoLayout* layout, const char* text, uint32_t* cellw, uint32_t* x, uint32_t* y)
{
    assert(layout), assert(text), assert(cellw), assert(x), assert(y);
    if (*cellw == 0) return false;

    int w, h;
    pango_layout_set_text(layout, text, -1);
    pango_layout_get_pixel_size(layout, &w, &h);

    if ((uint32_t)w > *cellw)
    {
        drawTruncatedText(layout, text, *cellw);
        *cellw = 0;
        goto end;
    }
    else
    {
        *cellw -= w;
    }

end:
    cairo_move_to(cairo->cr, *x, *y);
    pango_cairo_show_layout(cairo->cr, layout);
    *x += w;
    return *cellw != 0;
}

static bool
drawModText(PangoLayout* layout, uint32_t idx, uint32_t* cellw, uint32_t* x, uint32_t* y)
{
    assert(layout), assert(cellw), assert(x), assert(y);
    if (!setSourceRgba(MENU_COLOR_KEY)) return false;

    Modifiers* mods = &mainMenu->keyChords[idx].key.mods;
    if (mods->ctrl && !drawText(layout, "C-", cellw, x, y)) return false;
    if (mods->alt && !drawText(layout, "M-", cellw, x, y)) return false;
    if (mods->hyper && !drawText(layout, "H-", cellw, x, y)) return false;
    if (mods->shift && !drawText(layout, "S-", cellw, x, y)) return false;

    return true;
}

static bool
drawKeyText(PangoLayout* layout, uint32_t idx, uint32_t* cellw, uint32_t* x, uint32_t* y)
{
    assert(layout), assert(cellw), assert(x), assert(y);
    if (!setSourceRgba(MENU_COLOR_KEY)) return false;

    return drawText(layout, mainMenu->keyChords[idx].key.repr, cellw, x, y);
}

static bool
drawDelimiterText(PangoLayout* layout, uint32_t* cellw, uint32_t* x, uint32_t* y)
{
    assert(layout), assert(cellw), assert(x), assert(y);
    if (!setSourceRgba(MENU_COLOR_DELIMITER)) return false;

    return drawText(layout, mainMenu->delimiter, cellw, x, y);
}

static bool
drawDescriptionText(PangoLayout* layout, uint32_t idx, uint32_t* cellw, uint32_t* x, uint32_t* y)
{
    assert(layout), assert(cellw), assert(x), assert(y);
    if (!setSourceRgba(
            mainMenu->keyChords[idx].keyChords ? MENU_COLOR_PREFIX : MENU_COLOR_CHORD
        )) return false;

    return drawText(layout, mainMenu->keyChords[idx].description, cellw, x, y);
}

static void
drawHintText(PangoLayout* layout, uint32_t idx, uint32_t cellw, uint32_t x, uint32_t y)
{
    assert(layout);

    if (!drawModText(layout, idx, &cellw, &x, &y)) return;
    if (!drawKeyText(layout, idx, &cellw, &x, &y)) return;
    if (!drawDelimiterText(layout, &cellw, &x, &y)) return;
    if (!drawDescriptionText(layout, idx, &cellw, &x, &y)) return;
}

static bool
drawGrid()
{
    assert(cairo), assert(mainMenu);

    if (mainMenu->borderWidth * 2 >= width)
    {
        errorMsg("Border is larger than menu width.");
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

    if (!setSourceRgba(MENU_COLOR_KEY)) goto fail;

    if (mainMenu->debug)
    {
        disassembleGrid(startx, starty, rows, cols, wpadding, hpadding, cellWidth, cellHeight, count);
        disassembleKeyChordsShallow(mainMenu->keyChords, mainMenu->keyChordCount);
    }

    if (!ellipsisIsSet)
    {
        ellipsisIsSet = true;
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
            drawHintText(layout, idx, cellWidth - (wpadding * 2), x, y);
            /* cairo_move_to(cairo->cr, x, y); */
            /* drawHintText(layout, mainMenu->keyChords[idx].hint, cellWidth - (wpadding * 2)); */
            /* pango_cairo_show_layout(cairo->cr, layout); */
        }
    }

    g_object_unref(layout);
    return true;

fail:
    g_object_unref(layout);
end:
    return false;
}

bool
cairoPaint(Cairo* cr, Menu* menu)
{
    assert(cr), assert(menu);

    if (menuIsDelayed(menu)) return true;

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

    return true;

fail:
    return false;
}
