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
#include "common/array.h"
#include "common/common.h"
#include "common/debug.h"
#include "common/key_chord.h"
#include "common/menu.h"
#include "common/string.h"

/* local includes */
#include "cairo.h"

/* Drawing context to store ellipsis state */
typedef struct
{
    int  ellipsisWidth;
    int  ellipsisHeight;
    bool ellipsisIsSet;
} DrawingContext;

static void
initDrawingContext(DrawingContext* ctx)
{
    assert(ctx);

    ctx->ellipsisWidth  = -1;
    ctx->ellipsisHeight = -1;
    ctx->ellipsisIsSet  = false;
}

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

static uint32_t
getFontHeight(cairo_t* cr, const char* font)
{
    assert(cr), assert(font);

    PangoFontDescription* fontDesc = pango_font_description_from_string(font);
    PangoLayout*          layout   = pango_cairo_create_layout(cr);
    PangoRectangle        rect;

    pango_layout_set_text(
        layout,
        "!\"#$%%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~",
        -1);
    pango_layout_set_font_description(layout, fontDesc);
    pango_layout_set_single_paragraph_mode(layout, 1);
    pango_layout_get_pixel_extents(layout, NULL, &rect);

    /* cleanup */
    pango_font_description_free(fontDesc);
    g_object_unref(layout);

    return rect.height;
}

uint32_t
cairoGetHeight(Menu* menu, cairo_surface_t* surface, uint32_t maxHeight)
{
    assert(menu), assert(surface);

    uint32_t height = 0;

    cairo_t* cr          = cairo_create(surface);
    uint32_t cellHeight  = getFontHeight(cr, menu->font) + (menu->hpadding * 2);
    uint32_t titleHeight = getFontHeight(cr, menu->titleFont) + (menu->hpadding * 2);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    menu->cellHeight  = cellHeight;
    menu->titleHeight = (menu->title && strlen(menu->title) > 0) ? titleHeight : 0;
    calculateGrid(menu->keyChords->length, menu->maxCols, &menu->rows, &menu->cols);

    /* Calculate table padding for height calculation - if -1, use cell padding, otherwise use the
     * specified value */
    uint32_t tablePadding = (menu->tablePadding == -1)
                                ? menu->hpadding
                                : (menu->tablePadding < 0 ? 0U : (uint32_t)menu->tablePadding);

    height = menu->titleHeight +
             (menu->cellHeight * menu->rows) +
             (tablePadding * 2) +
             (menu->borderWidth * 2);

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

    /* foreground - title */
    paint->fgTitle.r = (float)colors[MENU_COLOR_TITLE].r / 255.0f;
    paint->fgTitle.g = (float)colors[MENU_COLOR_TITLE].g / 255.0f;
    paint->fgTitle.b = (float)colors[MENU_COLOR_TITLE].b / 255.0f;
    paint->fgTitle.a = (float)colors[MENU_COLOR_TITLE].a / 255.0f;

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
    paint->font      = menu->font;
    paint->titleFont = menu->titleFont;
}

static bool
setSourceRgba(cairo_t* cr, CairoPaint* paint, MenuColor type)
{
    assert(cr), assert(paint);

    CairoColor* color = NULL;

    switch (type)
    {
    case MENU_COLOR_KEY: color = &paint->fgKey; break;
    case MENU_COLOR_DELIMITER: color = &paint->fgDelimiter; break;
    case MENU_COLOR_PREFIX: color = &paint->fgPrefix; break;
    case MENU_COLOR_CHORD: color = &paint->fgChord; break;
    case MENU_COLOR_TITLE: color = &paint->fgTitle; break;
    case MENU_COLOR_BACKGROUND: color = &paint->bg; break;
    case MENU_COLOR_BORDER: color = &paint->bd; break;
    default: errorMsg("Invalid color request %d", type); return false;
    }

    cairo_set_source_rgba(cr, color->r, color->g, color->b, color->a);
    return true;
}

static void
drawRoundedPath(cairo_t* cr, double x, double y, double w, double h, double radius)
{
    assert(cr);

    double degrees = M_PI / 180;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - radius, y + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc(cr, x + w - radius, y + h - radius, radius, 0 * degrees, 90 * degrees);
    cairo_arc(cr, x + radius, y + h - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);
}

static bool
drawBackground(cairo_t* cr, CairoPaint* paint, Menu* menu, uint32_t width, uint32_t height)
{
    assert(cr), assert(paint), assert(menu);

    if (!setSourceRgba(cr, paint, MENU_COLOR_BACKGROUND)) return false;

    double radius = menu->borderRadius;

    if (!radius)
    {
        cairo_paint(cr);
    }
    else
    {
        double x = menu->borderWidth / 2.0;
        double y = menu->borderWidth / 2.0;
        double w = width - menu->borderWidth;
        double h = height - menu->borderWidth;
        drawRoundedPath(cr, x, y, w, h, radius);
        cairo_fill(cr);
    }

    return true;
}

static bool
drawBorder(cairo_t* cr, CairoPaint* paint, Menu* menu, uint32_t width, uint32_t height)
{
    assert(cr), assert(paint), assert(menu);

    double lineW = cairo_get_line_width(cr);
    cairo_set_line_width(cr, menu->borderWidth);
    if (!setSourceRgba(cr, paint, MENU_COLOR_BORDER)) return false;

    double radius = menu->borderRadius;

    double x = menu->borderWidth / 2.0;
    double y = menu->borderWidth / 2.0;
    double w = width - menu->borderWidth;
    double h = height - menu->borderWidth;
    if (!radius)
    {
        cairo_rectangle(cr, x, y, w, h);
    }
    else
    {
        drawRoundedPath(cr, x, y, w, h, radius);
    }

    cairo_stroke(cr);
    cairo_set_line_width(cr, lineW);
    return true;
}

static void
initEllipsisIfNeeded(cairo_t* cr, PangoLayout* layout, DrawingContext* ctx)
{
    assert(cr), assert(layout), assert(ctx);

    if (!ctx->ellipsisIsSet)
    {
        ctx->ellipsisIsSet = true;
        pango_layout_set_text(layout, "...", -1);
        pango_layout_get_pixel_size(layout, &ctx->ellipsisWidth, &ctx->ellipsisHeight);
    }
}

static void
drawTruncatedText(PangoLayout* layout, const char* text, uint32_t cellw, int ellipsisWidth)
{
    assert(layout), assert(text);
    if ((uint32_t)ellipsisWidth > cellw) return;

    size_t   len = strlen(text);
    int      textw;
    int      texth;
    uint32_t truncatedWidth = 0;
    char     buffer[len + 1];      /* +1 for null byte '\0' */
    memcpy(buffer, text, len + 1); /* +1 to copy null byte '\0' */

    pango_layout_set_text(layout, buffer, len);
    pango_layout_get_pixel_size(layout, &textw, &texth);

    truncatedWidth = textw + ellipsisWidth;
    if (truncatedWidth <= cellw) return;

    size_t left  = 0;
    size_t right = len;
    while (left < right)
    {
        while (left < right && isUtf8ContByte(buffer[left]))
            left++;
        size_t mid = (left + right) / 2;
        while (mid > left && !isUtf8StartByte(buffer[mid]))
            mid--;

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
        while (len && !isUtf8StartByte(buffer[len]))
            len--;
    }

    memcpy(buffer + len, "...", 4);
    pango_layout_set_text(layout, buffer, -1);
}

static bool
drawText(
    cairo_t*     cr,
    PangoLayout* layout,
    const char*  text,
    uint32_t*    cellw,
    uint32_t*    x,
    uint32_t*    y,
    int          ellipsisWidth)
{
    assert(cr), assert(layout), assert(text), assert(cellw), assert(x), assert(y);
    if (*cellw == 0) return false;
    if ((uint32_t)ellipsisWidth > *cellw) return false;

    int w, h;
    pango_layout_set_text(layout, text, -1);
    pango_layout_get_pixel_size(layout, &w, &h);

    if ((uint32_t)w > *cellw)
    {
        drawTruncatedText(layout, text, *cellw, ellipsisWidth);
        *cellw = 0;
    }
    else
    {
        *cellw -= w;
    }

    cairo_move_to(cr, *x, *y);
    pango_cairo_show_layout(cr, layout);
    *x += w;
    return *cellw != 0;
}

static bool
drawString(
    cairo_t*      cr,
    PangoLayout*  layout,
    const String* string,
    uint32_t*     cellw,
    uint32_t*     x,
    uint32_t*     y,
    int           ellipsisWidth)
{
    assert(cr), assert(layout), assert(string), assert(cellw), assert(x), assert(y);

    char buffer[string->length + 1];
    stringWriteToBuffer(string, buffer);
    return drawText(cr, layout, buffer, cellw, x, y, ellipsisWidth);
}

static bool
drawTitleText(
    cairo_t*    cr,
    CairoPaint* paint,
    Menu*       menu,
    uint32_t*   yOffset,
    uint32_t    cellw,
    uint32_t    x,
    uint32_t    y,
    int         ellipsisWidth)
{
    assert(cr), assert(paint), assert(menu), assert(yOffset);

    *yOffset = 0;
    if (!menu->title || strlen(menu->title) == 0) return true;

    if (!setSourceRgba(cr, paint, MENU_COLOR_TITLE)) return false;

    PangoFontDescription* fontDesc = pango_font_description_from_string(paint->titleFont);
    PangoLayout*          layout   = pango_cairo_create_layout(cr);

    pango_layout_set_font_description(layout, fontDesc);

    int textw, texth;
    pango_layout_set_text(layout, menu->title, -1);
    pango_layout_get_pixel_size(layout, &textw, &texth);

    uint32_t centeredX = x;
    if ((uint32_t)textw < cellw)
    {
        centeredX = x + (cellw - (uint32_t)textw) / 2;
    }

    uint32_t titleX = centeredX;
    uint32_t titleY = y;
    drawText(cr, layout, menu->title, &cellw, &titleX, &titleY, ellipsisWidth);

    *yOffset = texth + menu->hpadding;

    pango_font_description_free(fontDesc);
    g_object_unref(layout);

    return true;
}

static bool
drawKeyModText(
    cairo_t*     cr,
    CairoPaint*  paint,
    PangoLayout* layout,
    const Key*   key,
    uint32_t*    cellw,
    uint32_t*    x,
    uint32_t*    y,
    int          ellipsisWidth)
{
    assert(cr), assert(paint), assert(layout), assert(key), assert(cellw), assert(x), assert(y);
    if (!setSourceRgba(cr, paint, MENU_COLOR_KEY)) return false;
    if (!modifierHasAnyActive(key->mods)) return true;

    bool status = true;
    if (modifierIsActive(key->mods, MOD_CTRL))
    {
        status = drawText(cr, layout, "C-", cellw, x, y, ellipsisWidth);
    }
    if (status && modifierIsActive(key->mods, MOD_META))
    {
        status = drawText(cr, layout, "M-", cellw, x, y, ellipsisWidth);
    }
    if (status && modifierIsActive(key->mods, MOD_HYPER))
    {
        status = drawText(cr, layout, "H-", cellw, x, y, ellipsisWidth);
    }
    if (status && modifierIsActive(key->mods, MOD_SHIFT))
    {
        status = drawText(cr, layout, "S-", cellw, x, y, ellipsisWidth);
    }
    return status;
}

static bool
drawKeyText(
    cairo_t*      cr,
    CairoPaint*   paint,
    PangoLayout*  layout,
    const String* string,
    uint32_t*     cellw,
    uint32_t*     x,
    uint32_t*     y,
    int           ellipsisWidth)
{
    assert(cr), assert(paint), assert(layout), assert(string), assert(cellw), assert(x), assert(y);
    if (!setSourceRgba(cr, paint, MENU_COLOR_KEY)) return false;

    return drawString(cr, layout, string, cellw, x, y, ellipsisWidth);
}

static bool
drawDelimiterText(
    cairo_t*     cr,
    CairoPaint*  paint,
    PangoLayout* layout,
    const char*  delimiter,
    uint32_t*    cellw,
    uint32_t*    x,
    uint32_t*    y,
    int          ellipsisWidth)
{
    assert(cr), assert(paint), assert(layout), assert(delimiter), assert(cellw), assert(x),
        assert(y);
    if (!setSourceRgba(cr, paint, MENU_COLOR_DELIMITER)) return false;

    return drawText(cr, layout, delimiter, cellw, x, y, ellipsisWidth);
}

static bool
drawDescriptionText(
    cairo_t*        cr,
    CairoPaint*     paint,
    PangoLayout*    layout,
    const KeyChord* keyChord,
    uint32_t*       cellw,
    uint32_t*       x,
    uint32_t*       y,
    int             ellipsisWidth)
{
    assert(cr), assert(paint), assert(layout), assert(keyChord), assert(cellw), assert(x), assert(y);

    if (!setSourceRgba(
            cr,
            paint,
            arrayIsEmpty(&keyChord->keyChords) ? MENU_COLOR_CHORD : MENU_COLOR_PREFIX))
        return false;

    return drawString(cr, layout, &keyChord->description, cellw, x, y, ellipsisWidth);
}

static void
drawHintText(
    cairo_t*        cr,
    CairoPaint*     paint,
    PangoLayout*    layout,
    const char*     delimiter,
    const KeyChord* keyChord,
    uint32_t        cellw,
    uint32_t        x,
    uint32_t        y,
    int             ellipsisWidth)
{
    assert(cr), assert(paint), assert(layout), assert(delimiter), assert(keyChord);

    if (!drawKeyModText(cr, paint, layout, &keyChord->key, &cellw, &x, &y, ellipsisWidth)) return;
    if (!drawKeyText(cr, paint, layout, &keyChord->key.repr, &cellw, &x, &y, ellipsisWidth)) return;
    if (!drawDelimiterText(cr, paint, layout, delimiter, &cellw, &x, &y, ellipsisWidth)) return;
    if (!drawDescriptionText(cr, paint, layout, keyChord, &cellw, &x, &y, ellipsisWidth)) return;
}

static bool
drawGrid(
    cairo_t*        cr,
    CairoPaint*     paint,
    Menu*           menu,
    uint32_t        width,
    uint32_t        height,
    DrawingContext* ctx)
{
    assert(cr), assert(paint), assert(menu);

    if (menu->borderWidth * 2 >= width)
    {
        errorMsg("Border is larger than menu width.");
        return false;
    }

    uint32_t tablePaddingX     = (menu->tablePadding == -1)
                                     ? menu->wpadding
                                     : (menu->tablePadding < 0 ? 0U : (uint32_t)menu->tablePadding);
    uint32_t tablePaddingY     = (menu->tablePadding == -1)
                                     ? menu->hpadding
                                     : (menu->tablePadding < 0 ? 0U : (uint32_t)menu->tablePadding);
    uint32_t startx            = menu->borderWidth + tablePaddingX;
    uint32_t starty            = menu->borderWidth + tablePaddingY;
    uint32_t rows              = menu->rows;
    uint32_t cols              = menu->cols;
    uint32_t wpadding          = menu->wpadding;
    uint32_t hpadding          = menu->hpadding;
    uint32_t totalTablePadding = tablePaddingX + tablePaddingY;
    uint32_t borderWidthTotal  = menu->borderWidth * 2;
    uint32_t availableWidth    = (totalTablePadding > (width - borderWidthTotal))
                                     ? 0
                                     : width - borderWidthTotal - totalTablePadding;
    uint32_t cellWidth         = (availableWidth > 0) ? availableWidth / cols : 0;
    uint32_t cellHeight        = menu->cellHeight;
    uint32_t count             = menu->keyChords->length;

    PangoFontDescription* fontDesc = pango_font_description_from_string(menu->font);
    PangoLayout*          layout   = pango_cairo_create_layout(cr);

    pango_layout_set_font_description(layout, fontDesc);
    pango_font_description_free(fontDesc);

    if (!setSourceRgba(cr, paint, MENU_COLOR_KEY)) goto fail;

    if (menu->debug)
    {
        disassembleGrid(
            startx,
            starty,
            rows,
            cols,
            wpadding,
            hpadding,
            cellWidth,
            cellHeight,
            count);
        disassembleKeyChordArrayShallow(menu->keyChords);
    }

    initEllipsisIfNeeded(cr, layout, ctx);

    if ((wpadding * 2) >= cellWidth)
    {
        errorMsg("Width padding is larger than cell size. Unable to draw anything.");
        goto fail;
    }

    if ((uint32_t)ctx->ellipsisWidth > cellWidth - (wpadding * 2))
    {
        warnMsg("Not enough cell space to draw truncated hints.");
        ctx->ellipsisWidth  = 0;
        ctx->ellipsisHeight = -1;
    }

    uint32_t titleOffset = 0;

    if (!drawTitleText(
            cr,
            paint,
            menu,
            &titleOffset,
            (availableWidth > 0 ? availableWidth - (wpadding * 2) : 0),
            startx,
            starty,
            ctx->ellipsisWidth))
    {
        errorMsg("Failed to draw menu title.");
        goto fail;
    }

    starty += titleOffset;

    ArrayIterator   iter     = arrayIteratorMake(menu->keyChords);
    const KeyChord* keyChord = NULL;
    for (uint32_t i = 0; i < cols; i++)
    {
        uint32_t x = startx + (i * cellWidth) + wpadding;
        for (uint32_t j = 0; j < rows; j++)
        {
            keyChord = ARRAY_ITER_NEXT(&iter, const KeyChord);
            if (!keyChord) goto done;

            uint32_t y = starty + (j * cellHeight) + hpadding;
            drawHintText(
                cr,
                paint,
                layout,
                menu->delimiter,
                keyChord,
                cellWidth - (wpadding * 2),
                x,
                y,
                ctx->ellipsisWidth);
        }
    }

done:
    g_object_unref(layout);
    return true;

fail:
    g_object_unref(layout);
    return false;
}

bool
cairoPaint(Cairo* cairo, Menu* menu)
{
    assert(cairo), assert(menu);

    if (menu->debug) disassembleMenu(menu);
    if (arrayIsEmpty(menu->keyChords)) return false;
    if (menuIsDelayed(menu)) return true;

    /* Clear delay after first successful render */
    if (menu->delay) menu->delay = 0;

    uint32_t       width  = menu->width;
    uint32_t       height = menu->height / cairo->scale;
    DrawingContext ctx;
    initDrawingContext(&ctx);

    if (!drawBackground(cairo->cr, cairo->paint, menu, width, height))
    {
        errorMsg("Could not draw background.");
        return false;
    }

    if (!drawBorder(cairo->cr, cairo->paint, menu, width, height))
    {
        errorMsg("Could not draw border.");
        return false;
    }

    if (!drawGrid(cairo->cr, cairo->paint, menu, width, height, &ctx))
    {
        errorMsg("Could not draw grid.");
        return false;
    }

    return true;
}
