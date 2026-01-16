#ifndef WK_RUNTIME_CAIRO_H_
#define WK_RUNTIME_CAIRO_H_

#include <cairo.h>

#include "common/menu.h"

typedef struct
{
    float r, g, b, a;
} CairoColor;

typedef struct
{
    CairoColor  fgKey;
    CairoColor  fgDelimiter;
    CairoColor  fgPrefix;
    CairoColor  fgChord;
    CairoColor  fgTitle;
    CairoColor  fgGoto;
    CairoColor  bg;
    CairoColor  bd;
    const char* font;
    const char* titleFont;
} CairoPaint;

typedef struct
{
    cairo_t*         cr;
    cairo_surface_t* surface;
    CairoPaint*      paint;
    double           scale;
    uint32_t         width;
    uint32_t         height;
} Cairo;

bool     cairoCreateForSurface(Cairo* cairo, cairo_surface_t* surface);
void     cairoDestroy(Cairo* cairo);
uint32_t cairoHeight(Menu* menu, cairo_surface_t* surface, uint32_t maxHeight);
void     cairoPaintInit(Menu* menu, CairoPaint* paint);
bool     cairoPaint(Cairo* cairo, Menu* menu);

#endif /* WK_RUNTIME_CAIRO_H_ */
