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
#include "util.h"
#include "window.h"

bool
cairoCreateForSurface(Cairo* cairo, cairo_surface_t* surface)
{
    assert(cairo && surface);

    cairo->cr = cairo_create(surface);
    if (!cairo->cr) goto fail;

    cairo->pango = pango_cairo_create_context(cairo->cr);
    if (!cairo->pango) goto fail;

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
cairoGetHeight(WkProperties* properties, cairo_surface_t* surface)
{
    assert(properties && surface);

    uint32_t count = countChords(properties->chords);
    uint32_t rows = 0;
    uint32_t cols = 0;

    cairo_t* cr = cairo_create(surface);
    PangoLayout* layout = pango_cairo_create_layout(cr);
    PangoFontDescription* fontDesc = pango_font_description_from_string(properties->font);
    PangoRectangle rect;

    calculateGrid(count, properties->maxCols, &rows, &cols);

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
    cairo_surface_destroy(surface);
    cairo_destroy(cr);
    g_object_unref(layout);
    pango_font_description_free(fontDesc);

    return (rect.height + properties->heightPadding * 2) * rows;
}

static PangoLayout*
pangoGetLayout(Cairo* cairo, CairoPaint* paint, const char* buffer)
{
    PangoLayout* layout = pango_cairo_create_layout(cairo->cr);
    pango_layout_set_text(layout, buffer, -1);
    PangoFontDescription* desc = pango_font_description_from_string(paint->font);
    pango_layout_set_font_description(layout, desc);
    pango_layout_set_single_paragraph_mode(layout, 1);
    pango_font_description_free(desc);
    return layout;
}

static bool
pangoGetTextExtents(Cairo* cairo, CairoPaint* paint, CairoResult* result, const char* fmt, ...)
{
    assert(cairo && paint && result && fmt);
    memset(result, 0, sizeof(CairoResult));

    va_list args;
    va_start(args, fmt);
    /* FIXME */
    bool ret = vrprintf(NULL, NULL, fmt, args);
    va_end(args);

    if (!ret) return false;

    PangoRectangle rect;
    PangoLayout *layout = pangoGetLayout(cairo, paint, NULL);
    pango_layout_get_pixel_extents(layout, NULL, &rect);
    int baseline = pango_layout_get_baseline(layout) / PANGO_SCALE;
    g_object_unref(layout);

    result->xAdvance = rect.x + rect.width;
    result->height = rect.height;
    result->baseline = baseline;
    return true;
}

static void
setColors(CairoPaint* paint, WkHexColor* colors)
{
    /* foreground */
    paint->fg.r = colors[WK_COLOR_FOREGROUND].r;
    paint->fg.g = colors[WK_COLOR_FOREGROUND].g;
    paint->fg.b = colors[WK_COLOR_FOREGROUND].b;
    paint->fg.a = colors[WK_COLOR_FOREGROUND].a;

    /* background */
    paint->bg.r = colors[WK_COLOR_BACKGROUND].r;
    paint->bg.g = colors[WK_COLOR_BACKGROUND].g;
    paint->bg.b = colors[WK_COLOR_BACKGROUND].b;
    paint->bg.a = colors[WK_COLOR_BACKGROUND].a;

    /* border */
    paint->bd.r = colors[WK_COLOR_BORDER].r;
    paint->bd.g = colors[WK_COLOR_BORDER].g;
    paint->bd.b = colors[WK_COLOR_BORDER].b;
    paint->bd.a = colors[WK_COLOR_BORDER].a;
}

void
cairoPaint(
    Cairo* cairo,
    uint32_t width,
    uint32_t maxHeight,
    WkProperties* properties,
    CairoPaintResult* outResult
)
{
    assert(cairo && properties && outResult);

    maxHeight /= cairo->scale;

    memset(outResult, 0, sizeof(CairoPaintResult));
    outResult->displayed = 1;

    CairoPaint paint = {0};
    paint.font = properties->font;

    CairoResult result = {0};
    uint32_t asciiHeight;
    pangoGetTextExtents(cairo, &paint, &result, "!\"#$%%&'()*+,-./0123456789:;<=>?@ABCD"
                        "EFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");

    asciiHeight = result.height;
    paint.baseline = result.baseline;

    size_t borderWidth = properties->borderWidth;
    width -= borderWidth;
    uint32_t height = MIN(asciiHeight, maxHeight);
    uint32_t vpadding = (height - asciiHeight) / 2;
    /* double borderRadius = 0; /\* TODO decide if there should be support for curved borders...curved...borders. *\/ */
    uint32_t chordsCount = countChords(properties->chords);

    cairo_set_source_rgba(cairo->cr, 0, 0, 0, 0);
    cairo_rectangle(cairo->cr, 0, 0, width, height);

    cairo_save(cairo->cr);
    cairo_set_operator(cairo->cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cairo->cr);
    cairo_restore(cairo->cr);

    /* uint32_t count; */
    uint32_t rows = 0;
    uint32_t cols = 0;
    calculateGrid(chordsCount, properties->maxCols, &rows, &cols); /* rows i.e. lines */

    memset(&result, 0, sizeof(result));
    setColors(&paint, properties->colors);

    paint.pos = (struct pos){ 0 + result.xAdvance + borderWidth, vpadding + borderWidth };
    paint.box = (struct box){ 4, 0, vpadding, -vpadding, width - paint.pos.x, height };

    uint32_t titleh = (result.height > 0 ? result.height : height); /* FIXME no title sooo..... */
    outResult->height = titleh;

    /* uint32_t posy = titleh; */
    /* uint32_t spacingX = 0; */
    /* uint32_t spacingY = 0; */
}
