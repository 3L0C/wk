#ifndef WK_LIB_CAIRO_H_
#define WK_LIB_CAIRO_H_

#include <cairo.h>

#include "common.h"
#include "properties.h"

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
} CairoPaint;

typedef struct
{
    cairo_t* cr;
    cairo_surface_t* surface;
    CairoPaint* paint;
    uint32_t scale;
    uint32_t width;
    uint32_t height;
} Cairo;

bool cairoCreateForSurface(Cairo* cairo, cairo_surface_t* surface);
void cairoDestroy(Cairo* cairo);
uint32_t cairoGetHeight(WkProperties* props, cairo_surface_t* surface, uint32_t maxHeight);
void cairoInitPaint(WkProperties* props, CairoPaint* paint);
void cairoPaint(Cairo* cairo, WkProperties* props);

#endif /* WK_LIB_CAIRO_H_ */
