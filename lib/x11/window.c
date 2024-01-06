#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <sysexits.h>

#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>

#include "cairo.h"
#include "lib/cairo.h"
#include "lib/common.h"
#include "lib/window.h"

#include "window.h"

static X11 x11;
static WkX11Window* window = &x11.window;
static WkProperties* properties;

static void
checkLocale(void)
{
    if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
    {
        warnMsg("Locale not supported.");
    }
}

static bool
x11Position(WkWindowPosition pos)
{
    assert(x11.props);
    return x11.props->position == pos;
}
static uint32_t
getWindowWidth(void)
{
    uint32_t width = window->width * (window->widthFactor ? window->widthFactor : 1);

    if (width > window->width - 2 * window->hmarginSize)
    {
        width = window->width - 2 * window->hmarginSize;
    }

    if (width < WINDOW_MIN_WIDTH || 2 * window->hmarginSize > window->width)
    {
        width = WINDOW_MIN_WIDTH;
    }

    return width;
}

static void
setMonitor(void)
{
    static int32_t monitor = -1;
    int propWidth = properties->desiredWidth;
    int propHeight = properties->desiredHeight;
    Window root = DefaultRootWindow(window->display);

    {
#define INTERSECT(x,y,w,h,r)                                                 \
        (fmax(0, fmin((x)+(w),(r).x_org+(r).width) - fmax((x),(r).x_org)) && \
         fmax(0, fmin((y)+(h),(r).y_org+(r).height) - fmax((y),(r).y_org)))

        int32_t n;
        XineramaScreenInfo* info = XineramaQueryScreens(window->display, &n);
        if (info)
        {
            int32_t x, y, a, j, di, i = 0, area = 0;
            uint32_t du;
            Window w, pw, dw, *dws;
            XWindowAttributes wa;

            XGetInputFocus(window->display, &w, &di);
            if (monitor >= 0 && monitor < n)
            {
                i = monitor;
            }
            else if (w != root && w != PointerRoot && w != None)
            {
                /* find top-level window containing current input focus */
                do
                {
                    if (XQueryTree(window->display, (pw = w), &dw, &w, &dws, &du) && dws)
                    {
                        XFree(dws);
                    }
                }
                while (w != root && w != pw);

                /* find xinerama screen with which the window intersects most */
                if (XGetWindowAttributes(window->display, pw, &wa))
                {
                    for (j = 0; j < n; j++)
                    {
                        a = INTERSECT(wa.x, wa.y, wa.width, wa.height, info[j]);
                        if (a > area)
                        {
                            area = a;
                            i = j;
                        }
                    }
                }
            }

            /* no focused window is on screen, so use pointer location instead */
            if (monitor < 0 && !area &&
                XQueryPointer(window->display, root, &dw, &dw, &x, &y, &di, &di, &du))
            {
                for (i = 0; i < n; i++)
                {
                    if (INTERSECT(x, y, 1, 1, info[i]) > 0) break;
                }
            }

            if (propWidth == -1)
            {
                window->x = info[i].x_org + ((info[i].width / 2) / 2);
                window->width = info[i].width / 2;
            }
            else if (propWidth == 0 || propWidth > info[i].width)
            {
                window->x = info[i].x_org;
                window->width = info[i].width;
            }
            else
            {
                window->x = info[i].x_org + ((info[i].width - propWidth) / 2);
                window->width = propWidth;
            }

            if (propHeight == -1)
            {
                window->y = info[i].y_org + (
                    (info[i].height / 10) * (x11Position(WK_WIN_POS_BOTTOM) ? -1 : 1)
                );
            }
            else if (propHeight == 0 || propHeight > info[i].height)
            {
                window->y = info[i].y_org;
            }
            else
            {
                window->y = info[i].y_org + ((info[i].height - propHeight) / 2);
            }

            if (x11Position(WK_WIN_POS_BOTTOM))
            {
                window->y += info[i].height - window->height; /* FIXME need to calculate window height at some point */
            }

            window->maxHeight = info[i].height;
            XFree(info);
        }
        else
        {
            window->maxHeight = DisplayHeight(window->display, window->screen);
            window->x = 0;
            if (x11Position(WK_WIN_POS_BOTTOM))
            {
                window->y = window->maxHeight - window->height;
            }
            else
            {
                window->y = 0;
            }
            window->width = DisplayWidth(window->display, window->screen);
        }

        window->origWidth = window->width;
        window->origX = window->x;
        window->width = getWindowWidth();
        window->x += (window->origWidth - window->width) / 2;

#undef INTERSECT
    }

    window->monitor = monitor;
    XMoveResizeWindow(
        window->display, window->drawable, window->x, window->y + window->yOffset,
        window->width, window->height
    );
    XFlush(window->display);
}

static bool
initX11()
{
    Display* display = window->display = x11.dispaly = XOpenDisplay(NULL);
    if (!x11.dispaly) return false;
    window->screen = DefaultScreen(display);
    window->width = window->height = 1;
    window->monitor = -1;
    window->visual = DefaultVisual(display, window->screen);
    XSetWindowAttributes wa = {
        .event_mask = ExposureMask | KeyPressMask | ButtonPressMask | VisibilityChangeMask,
        .override_redirect = True
    };

    XVisualInfo vinfo;
    int depth = DefaultDepth(display, window->screen);
    unsigned long valuemask = CWOverrideRedirect | CWEventMask | CWBackPixel;

    if (XMatchVisualInfo(display, DefaultScreen(display), 32, TrueColor, &vinfo))
    {
        depth = vinfo.depth;
        window->visual = vinfo.visual;
        wa.background_pixmap = None;
        wa.border_pixel = 0;
        wa.colormap = XCreateColormap(display, DefaultRootWindow(display), window->visual, AllocNone);
    }

    window->drawable = XCreateWindow(
        display, DefaultRootWindow(display), 0, 0, window->width, window->height,
        0, depth, CopyFromParent, window->visual, valuemask, &wa
    );
    XSelectInput(display, window->drawable, ButtonPressMask | KeyPressMask);
    XMapRaised(display, window->drawable);
    window->xim = XOpenIM(display, NULL, NULL, NULL);
    window->xic = XCreateIC(
        window->xim,
        XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, window->drawable,
        XNFocusWindow, window->drawable,
        NULL
    );
    XSetClassHint(display, window->drawable, (XClassHint[]){{ .res_name = "wk", .res_class = "wk" }});
    setMonitor();
    window->render = cairoPaint;
    return true;
}

static void
destroyBuffer(Buffer* buffer)
{
    cairoDestroy(&buffer->cairo);
    memset(buffer, 0, sizeof(Buffer));
}

static bool
createBuffer(Buffer* buffer)
{
    cairo_surface_t* surf = cairo_xlib_surface_create(
        window->display, window->drawable, window->visual, window->width, window->height
    );

    if (!surf) goto fail;

    cairo_xlib_surface_set_size(surf, window->width, window->height);
    buffer->cairo.scale = 1;

    if (!cairoCreateForSurface(&buffer->cairo, surf))
    {
        cairo_surface_destroy(surf);
        goto fail;
    }

    buffer->width = window->width;
    buffer->height = window->height;
    buffer->created = true;
    return true;

fail:
    destroyBuffer(buffer);
    return false;
}

static Buffer*
nextBuffer(void)
{
    Buffer* buffer = &window->buffer;

    if (!buffer) return NULL;

    if (window->width != buffer->width || window->height != buffer->height)
    {
        destroyBuffer(buffer);
    }

    if (!buffer->created && !createBuffer(buffer)) return NULL;

    return buffer;
}

static bool
render(void)
{
    uint32_t oldw = window->width;
    uint32_t oldh = window->height;

    Buffer* buffer;
    for (int32_t tries = 0; tries < 2; tries++)
    {
        buffer = nextBuffer();
        if (!buffer)
        {
            errorMsg("Could not get next buffer.");
            return false;
        }

        if (!window->render) break;

        cairo_push_group(buffer->cairo.cr);
        CairoPaintResult result;
        window->render(&buffer->cairo, buffer->width, window->maxHeight, &result);
        window->displayed = result.displayed;
        cairo_pop_group_to_source(buffer->cairo.cr);

        if (window->height == result.height) break;

        window->height = result.height;
        destroyBuffer(buffer);
    }

    if (oldw != window->width || oldh != window->height)
    {
        uint32_t winY = 0;

        if (properties->position == WK_WIN_POS_BOTTOM)
        {
            winY = window->maxHeight - window->height; /* FIXME */
        }

        XMoveResizeWindow(
            window->display, window->drawable, window->x, winY + window->yOffset,
            window->width, window->height
        );
    }

    if (buffer->created)
    {
        cairo_save(buffer->cairo.cr);
        cairo_set_operator(buffer->cairo.cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint(buffer->cairo.cr);
        cairo_surface_flush(buffer->cairo.surface);
        cairo_restore(buffer->cairo.cr);
    }

    return true;
}

int
runX11(WkProperties* props)
{
    assert(props);
    checkLocale();
    properties = x11.props = props;
    if (!initX11()) return EX_SOFTWARE;
    if (false) render();

    printf("x11\n");
    return EX_OK;
}
