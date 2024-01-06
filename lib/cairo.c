#include "cairo.h"
#include "pango/pangocairo.h"
#include <assert.h>
#include <stdbool.h>

void
cairoDestroy(Cairo* cairo)
{
    if (cairo->cr) cairo_destroy(cairo->cr);
    if (cairo->surface) cairo_surface_destroy(cairo->surface);
}

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
