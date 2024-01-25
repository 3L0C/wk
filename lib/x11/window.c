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
#include "lib/properties.h"
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

static cairo_surface_t*
getThrowawaySurface(void)
{
    return cairo_xlib_surface_create(
        window->display, window->drawable, window->visual,
        window->width, window->height
    );
}

static bool
desiriedPos(WkWindowPosition pos)
{
    assert(x11.props);
    return x11.props->position == pos;
}

static void
resizeWinWidth(void)
{
    int32_t windowWidth = properties->windowWidth;
    struct display* root = &window->root;
    if (windowWidth < 0)
    {
        /* set width to half the size of the screen */
        window->x = root->x + (root->w / 4);
        window->width = root->w / 2;
    }
    else if (windowWidth == 0 || (uint32_t)windowWidth > root->w)
    {
        /* make the window as wide as the screen */
        window->x = root->x;
        window->width = root->w;
    }
    else
    {
        /* set the width to the desired user setting */
        window->x = root->x + ((root->w - windowWidth) / 2); /* position in the middle */
        window->width = windowWidth;
    }
}

static void
resizeWinHeight(void)
{
    int32_t windowGap = properties->windowGap;
    struct display* root = &window->root;
    window->maxHeight = root->h;

    if (windowGap < 0)
    {
        /* user wants a 1/10th gap between edge of screen and window*/
        window->y = root->y + (root->h / 10);
    }
    else if (windowGap == 0 || (uint32_t)windowGap > root->h)
    {
        /* user has no gap, or it is too large for the screen */
        window->y = root->y;
    }
    else
    {
        /* position window with desired gap, if any */
        window->y = root->y + windowGap;
    }

    /* sanity check that window is not too big */
    if (window->height >= root->h)
    {
        window->y = 0;
        window->height = root->h;
    }

    if (desiriedPos(WK_WIN_POS_BOTTOM))
    {
        window->y = root->h - window->height - window->y;
    }
}

static void
resizeWindow(void)
{
    resizeWinWidth();
    resizeWinHeight();
}

static void
setMonitor(void)
{
    static int32_t monitor = -1;
    Window root = DefaultRootWindow(window->display);

    {
#define INTERSECT(x,y,w,h,r)                                              \
        (MAX(0, MIN((x)+(w),(r).x_org+(r).width) - MAX((x),(r).x_org)) && \
         MAX(0, MIN((y)+(h),(r).y_org+(r).height) - MAX((y),(r).y_org)))

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

            window->root.x = info[i].x_org;
            window->root.y = info[i].y_org;
            window->root.w = info[i].width;
            window->root.h = info[i].height;
            XFree(info);
        }
        else
        {
            window->root.x = 0;
            window->root.y = 0;
            window->root.w = DisplayWidth(window->display, window->screen);
            window->root.h = DisplayHeight(window->display, window->screen);
        }

    window->height = cairoGetHeight(properties, getThrowawaySurface(), window->root.h);

    resizeWindow();
#undef INTERSECT
    }

    window->monitor = monitor;
    XMoveResizeWindow(
        window->display, window->drawable, window->x, window->y,
        window->width, window->height
    );
    XFlush(window->display);
}

static void
initBuffer(void)
{
    Buffer* buffer = &window->buffer;
    memset(buffer, 0, sizeof(Buffer));
}

static bool
initX11(void)
{
    Display* display = window->display = x11.dispaly = XOpenDisplay(NULL);
    if (!x11.dispaly) return false;
    window->screen = DefaultScreen(display);
    window->width = window->height = 1;
    window->border = properties->borderWidth;
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
        valuemask = CWOverrideRedirect | CWEventMask | CWBackPixmap | CWColormap | CWBorderPixel;
    }

    window->drawable = XCreateWindow(
        display, DefaultRootWindow(display), 0, 0, window->width, window->height,
        window->border, depth, CopyFromParent, window->visual, valuemask, &wa
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
    initBuffer();
    cairoInitPaint(&window->paint, properties);
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
    cairo_surface_t* surface = cairo_xlib_surface_create(
        window->display, window->drawable, window->visual, window->width, window->height
    );

    if (!surface) goto fail;

    cairo_xlib_surface_set_size(surface, window->width, window->height);
    buffer->cairo.scale = 1;

    if (!cairoCreateForSurface(&buffer->cairo, surface))
    {
        cairo_surface_destroy(surface);
        goto fail;
    }

    buffer->cairo.paint = &window->paint;
    buffer->width = window->width;
    buffer->height = window->height;
    buffer->created = true;
    return true;

fail:
    destroyBuffer(buffer);
    return false;
}

static Buffer*
getBuffer(void)
{
    Buffer* buffer = &window->buffer;

    if (!buffer) return NULL;

    if (window->height != buffer->height) destroyBuffer(buffer);

    if (!buffer->created && !createBuffer(buffer)) return NULL;

    return buffer;
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
bmRender(void)
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
        window->render(&buffer->cairo, buffer->width, window->maxHeight, properties, &result);
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
            window->display, window->drawable, window->x, winY,
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


static bool
render(void)
{
    uint32_t oldh = window->height;
    window->height = cairoGetHeight(properties, getThrowawaySurface(), window->root.h);
    resizeWinHeight();
    Buffer* buffer = getBuffer();

    if (!buffer)
    {
        errorMsg("Could not get buffer while rendering.");
        return false;
    }

    cairo_push_group(buffer->cairo.cr);
    CairoPaintResult result;
    window->render(&buffer->cairo, buffer->width, buffer->height, properties, &result);
    window->displayed = result.displayed;
    cairo_pop_group_to_source(buffer->cairo.cr);

    if (oldh != window->height)
    {
        XMoveResizeWindow(
            window->display, window->drawable, window->x, window->y,
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

static void
cleanup(void)
{
    XUngrabKey(window->display, AnyKey, AnyModifier, DefaultRootWindow(window->display));
    XSync(window->display, False);
    XCloseDisplay(window->display);
}

static bool
grabfocus(void)
{
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 10000000 };
    Window focuswin;
    int i, revertwin;

    for (i = 0; i < 100; i++)
    {
        XGetInputFocus(window->display, &focuswin, &revertwin);
        if (focuswin == window->drawable) return true;
        XSetInputFocus(window->display, window->drawable, RevertToParent, CurrentTime);
        nanosleep(&ts, NULL);
    }

    cleanup();
    errorMsg("Could not grab focus.");
    return false;
}

static bool
grabkeyboard(void)
{
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 };
    int i;

    /* try to grab keyboard, we may have to wait for another process to ungrab */
    for (i = 0; i < 1000; i++)
    {
        if (XGrabKeyboard(window->display,
                          DefaultRootWindow(window->display),
                          True, GrabModeAsync, GrabModeAsync,
                          CurrentTime) == GrabSuccess)
        {
            return true;
        }
        nanosleep(&ts, NULL);
    }

    cleanup();
    errorMsg("Could not grab keyboard.");
    return false;
}

static WkStatus
keypress(XKeyEvent* keyEvent)
{
    return WK_STATUS_EXIT_OK;
}

static int
eventHandler(void)
{
    XEvent ev;

    while (!XNextEvent(window->display, &ev))
    {
        if (XFilterEvent(&ev, window->drawable)) continue;

        switch (ev.type)
        {
        case DestroyNotify:
            if (ev.xdestroywindow.window != window->drawable) break;
            cleanup();
            return EX_SOFTWARE;
        case Expose:
            if (ev.xexpose.count == 0) render();
            break;
        case FocusIn:
            /* regrab focus from parent window */
            if (ev.xfocus.window != window->drawable)
            {
                if (!grabfocus()) return EX_SOFTWARE;
            }
            break;
        case KeyPress:
            return EX_OK;
            switch (keypress(&ev.xkey))
            {
            case WK_STATUS_RUNNING: break;
            case WK_STATUS_DAMAGED: render(); break;
            case WK_STATUS_EXIT_OK: return EX_OK;
            case WK_STATUS_EXIT_SOFTWARE: return EX_SOFTWARE;
            }
            break;
        case VisibilityNotify:
            if (ev.xvisibility.state != VisibilityUnobscured)
            {
                XRaiseWindow(window->display, window->drawable);
                XFlush(window->display);
            }
            break;
        }
    }

    return EX_OK;
}

int
runX11(WkProperties* props)
{
    assert(props);

    int result = EX_SOFTWARE;
    checkLocale();
    properties = x11.props = props;
    if (!initX11()) return result;
    grabkeyboard();
    render();
    result = eventHandler();

    if (false)
    {
        bmRender();
    }

    printf("x11\n");
    cleanup();
    return result;
}
