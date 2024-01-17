#ifndef WK_LIB_CAIRO_H_
#define WK_LIB_CAIRO_H_

#include <cairo.h>
#include <pango/pangocairo.h>
#include <stdint.h>

#include "propeties.h"

typedef struct
{
    cairo_t* cr;
    cairo_surface_t* surface;
    PangoContext* pango;
    int scale;
} Cairo;

typedef struct
{
    float r, g, b, a;
} CairoColor;

typedef struct
{
    CairoColor fg;
    CairoColor bg;
    CairoColor bd;
    const char* font;
    int32_t baseline;
    uint32_t hpadding;
    struct box
    {
        int32_t lx, rx; /* left/right offset (pos.x - lx, box.w + rx) */
        int32_t ty, by; /* top/bottom offset (pos.y - ty, box.h + by) */
        int32_t w, h;   /* 0 for text width/height */
    } box;

    struct pos
    {
        int32_t x, y;
    } pos;
} CairoPaint;

typedef struct
{
    uint32_t xAdvance;
    uint32_t height;
    uint32_t baseline;
} CairoResult;

typedef struct
{
    uint32_t displayed;
    uint32_t height;
} CairoPaintResult;

bool cairoCreateForSurface(Cairo* cairo, cairo_surface_t* surface);
void cairoDestroy(Cairo* cairo);
uint32_t cairoGetHeight(WkProperties* properties, cairo_surface_t* surface);
void cairoPaint(Cairo* cairo, uint32_t width, uint32_t maxHeight, WkProperties* properties, CairoPaintResult* outResult);

#endif /* WK_LIB_CAIRO_H_ */
