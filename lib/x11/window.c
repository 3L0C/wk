#include <assert.h>
#include <locale.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>

#include <cairo-xlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <unistd.h>

#include "lib/cairo.h"
#include "lib/common.h"
#include "lib/debug.h"
#include "lib/properties.h"
#include "lib/types.h"
#include "lib/util.h"

#include "debug.h"
#include "window.h"

typedef struct
{
    WkSpecial special;
    KeySym keysym;
} SpecialKey;

static X11 x11;
static WkX11Window* window = &x11.window;
static WkProperties* properties;
static bool debug;

static const SpecialKey specialkeys[] = {
    { WK_SPECIAL_NONE,      XK_VoidSymbol },
    { WK_SPECIAL_LEFT,      XK_Left },
    { WK_SPECIAL_LEFT,      XK_KP_Left },
    { WK_SPECIAL_RIGHT,     XK_Right },
    { WK_SPECIAL_RIGHT,     XK_KP_Right },
    { WK_SPECIAL_UP,        XK_Up },
    { WK_SPECIAL_UP,        XK_KP_Up },
    { WK_SPECIAL_DOWN,      XK_Down },
    { WK_SPECIAL_DOWN,      XK_KP_Down },
    { WK_SPECIAL_TAB,       XK_Tab },
    { WK_SPECIAL_TAB,       XK_KP_Tab },
    { WK_SPECIAL_SPACE,     XK_space },
    { WK_SPECIAL_SPACE,     XK_KP_Space },
    { WK_SPECIAL_RETURN,    XK_Return },
    { WK_SPECIAL_RETURN,    XK_KP_Enter },
    { WK_SPECIAL_DELETE,    XK_Delete },
    { WK_SPECIAL_DELETE,    XK_KP_Delete },
    { WK_SPECIAL_ESCAPE,    XK_Escape },
    { WK_SPECIAL_HOME,      XK_Home },
    { WK_SPECIAL_HOME,      XK_KP_Home },
    { WK_SPECIAL_PAGE_UP,   XK_Page_Up },
    { WK_SPECIAL_PAGE_UP,   XK_KP_Page_Up },
    { WK_SPECIAL_PAGE_DOWN, XK_Page_Down },
    { WK_SPECIAL_PAGE_DOWN, XK_KP_Page_Down },
    { WK_SPECIAL_END,       XK_End },
    { WK_SPECIAL_END,       XK_KP_End },
    { WK_SPECIAL_BEGIN,     XK_Begin },
    { WK_SPECIAL_BEGIN,     XK_KP_Begin }
};

static const size_t specialkeysLen = sizeof(specialkeys) / sizeof(specialkeys[0]);

static void
checkLocale(void)
{
    if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
    {
        warnMsg("Locale not supported.");
    }
    debugMsg(debug, "Locale supported.");
}

static cairo_surface_t*
getThrowawaySurface(void)
{
    return cairo_image_surface_create(CAIRO_FORMAT_ARGB32, window->width, window->height);
    /* return cairo_xlib_surface_create( */
    /*     window->display, window->drawable, window->visual, */
    /*     window->width, window->height */
    /* ); */
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
        window->y = (root->h / 10);
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
        window->y = root->h - window->height - window->y + root->y;
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
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
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
#undef MIN
#undef MAX
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
    debugMsg(debug, "Initializing x11.");
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
    initBuffer();
    cairoInitPaint(properties, &window->paint);
    if (debug) debugWindow(window);
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

static bool
render(void)
{
    uint32_t oldh = window->height;
    window->height = cairoGetHeight(properties, getThrowawaySurface(), window->root.h);
    resizeWinHeight();

    if (oldh != window->height)
    {
        XMoveResizeWindow(
            window->display, window->drawable, window->x, window->y,
            window->width, window->height
        );
    }

    Buffer* buffer = getBuffer();
    if (!buffer)
    {
        errorMsg("Could not get buffer while rendering.");
        return false;
    }

    properties->width = buffer->width;
    properties->height = buffer->height;
    window->render(&buffer->cairo, properties);
    cairo_surface_flush(buffer->cairo.surface);
    XFlush(window->display);

    return true;
}

static void
cleanup(X11* x)
{
    destroyBuffer(&x->window.buffer);
    XUngrabKey(x->window.display, AnyKey, AnyModifier, DefaultRootWindow(x->window.display));
    XSync(x->window.display, False);
    XCloseDisplay(x->window.display);
}

static void
cleanupAsync(void* xp)
{
    X11* x = (X11*)xp;
    close(ConnectionNumber(x->dispaly));
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

    cleanup(&x11);
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
        if (XGrabKeyboard(
                window->display,
                DefaultRootWindow(window->display),
                True, GrabModeAsync, GrabModeAsync, CurrentTime
            ) == GrabSuccess)
        {
            return true;
        }
        nanosleep(&ts, NULL);
    }

    cleanup(&x11);
    errorMsg("Could not grab keyboard.");
    return false;
}

static void
setKeyEventMods(WkMods* mods, unsigned int state)
{
    if (state & ControlMask) mods->ctrl = true;
    if (state & Mod1Mask) mods->alt = true;
    if (state & Mod4Mask) mods->hyper = true;
    if (state & ShiftMask) mods->shift = true;
}

static WkSpecial
getSpecialKey(KeySym keysym)
{
    for (size_t i = 0; i < specialkeysLen; i++)
    {
        if (specialkeys[i].keysym == keysym) return specialkeys[i].special;
    }
    return WK_SPECIAL_NONE;
}

static WkKeyType
processKey(Key* key, unsigned int state, KeySym keysym, const char* buffer, int len)
{
    key->key = buffer;
    key->len = len;
    setKeyEventMods(&key->mods, state);
    key->special = getSpecialKey(keysym);
    if (keyIsStrictlyMod(key)) return WK_KEY_IS_STRICTLY_MOD;
    if (keyIsSpecial(key)) return WK_KEY_IS_SPECIAL;
    if (keyIsNormal(key)) return WK_KEY_IS_NORMAL;
    return WK_KEY_IS_UNKNOWN;
    /* return (*key->key != '\0' || key->special != WK_SPECIAL_NONE); */
}

static WkStatus
keypress(XKeyEvent* keyEvent)
{
    KeySym keysym = XK_VoidSymbol;
    Status status;
    char buffer[32] = {0};
    int len;
    Key key = {0};
    unsigned int state = keyEvent->state;

    keyEvent->state &= ~(ControlMask);

    len = XmbLookupString(window->xic, keyEvent, buffer, sizeof(buffer), &keysym, &status);

    if (status == XLookupNone || status == XBufferOverflow) return WK_STATUS_RUNNING;

    switch (processKey(&key, state, keysym, buffer, len))
    {
    case WK_KEY_IS_STRICTLY_MOD: return WK_STATUS_RUNNING;
    case WK_KEY_IS_SPECIAL: /* FALLTHROUGH */
    case WK_KEY_IS_NORMAL: return handleKeypress(properties, &key);
    case WK_KEY_IS_UNKNOWN:
        debugMsg(debug, "Encountered an unknown key. Most likely a modifier key press event.");
        if (debug) debugKey(&key);
        return WK_STATUS_RUNNING;
    default: errorMsg("Got an unkown return value from 'processKey'."); break;
    }

    return WK_STATUS_EXIT_SOFTWARE;
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
            cleanup(&x11);
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
            switch (keypress(&ev.xkey))
            {
            case WK_STATUS_RUNNING: break;
            case WK_STATUS_DAMAGED: render(); break;
            case WK_STATUS_EXIT_OK: return EX_OK;
            case WK_STATUS_EXIT_SOFTWARE: return EX_SOFTWARE;
            }
            break;
        case ButtonPress: return EX_SOFTWARE;
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
    properties->cleanupfp = cleanupAsync;
    properties->xp = &x11;
    debug = properties->debug;
    if (!initX11()) return result;
    grabkeyboard();
    render();
    result = eventHandler();
    cleanup(&x11);
    return result;
}
