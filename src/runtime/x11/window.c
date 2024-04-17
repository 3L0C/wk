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

/* common includes */
#include "common/common.h"
#include "common/debug.h"
#include "common/menu.h"
#include "common/key_chord.h"
#include "common/util.h"

/* runtime includes */
#include "runtime/cairo.h"

/* local includes */
#include "debug.h"
#include "window.h"

typedef struct
{
    SpecialKey special;
    KeySym keysym;
} X11SpecialKey;

static X11 x11;
static X11Window* window = &x11.window;
static Menu* mainMenu;
static bool debug;

static const X11SpecialKey specialkeys[] = {
    { SPECIAL_KEY_NONE,      XK_VoidSymbol },
    { SPECIAL_KEY_LEFT,      XK_Left },
    { SPECIAL_KEY_LEFT,      XK_KP_Left },
    { SPECIAL_KEY_RIGHT,     XK_Right },
    { SPECIAL_KEY_RIGHT,     XK_KP_Right },
    { SPECIAL_KEY_UP,        XK_Up },
    { SPECIAL_KEY_UP,        XK_KP_Up },
    { SPECIAL_KEY_DOWN,      XK_Down },
    { SPECIAL_KEY_DOWN,      XK_KP_Down },
    { SPECIAL_KEY_TAB,       XK_Tab },
    { SPECIAL_KEY_TAB,       XK_KP_Tab },
    { SPECIAL_KEY_SPACE,     XK_space },
    { SPECIAL_KEY_SPACE,     XK_KP_Space },
    { SPECIAL_KEY_RETURN,    XK_Return },
    { SPECIAL_KEY_RETURN,    XK_KP_Enter },
    { SPECIAL_KEY_DELETE,    XK_Delete },
    { SPECIAL_KEY_DELETE,    XK_KP_Delete },
    { SPECIAL_KEY_ESCAPE,    XK_Escape },
    { SPECIAL_KEY_HOME,      XK_Home },
    { SPECIAL_KEY_HOME,      XK_KP_Home },
    { SPECIAL_KEY_PAGE_UP,   XK_Page_Up },
    { SPECIAL_KEY_PAGE_UP,   XK_KP_Page_Up },
    { SPECIAL_KEY_PAGE_DOWN, XK_Page_Down },
    { SPECIAL_KEY_PAGE_DOWN, XK_KP_Page_Down },
    { SPECIAL_KEY_END,       XK_End },
    { SPECIAL_KEY_END,       XK_KP_End },
    { SPECIAL_KEY_BEGIN,     XK_Begin },
    { SPECIAL_KEY_BEGIN,     XK_KP_Begin }
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
desiriedPos(MenuWindowPosition pos)
{
    assert(x11.menu);
    return x11.menu->position == pos;
}

static void
resizeWinWidth(void)
{
    int32_t windowWidth = mainMenu->windowWidth;
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
    int32_t windowGap = mainMenu->windowGap;
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

    if (desiriedPos(MENU_WIN_POS_BOTTOM))
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

    window->height = cairoGetHeight(mainMenu, getThrowawaySurface(), window->root.h);

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
    window->border = mainMenu->borderWidth;
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
    cairoInitPaint(mainMenu, &window->paint);
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
    window->height = cairoGetHeight(mainMenu, getThrowawaySurface(), window->root.h);
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

    mainMenu->width = buffer->width;
    mainMenu->height = buffer->height;
    if (!window->render(&buffer->cairo, mainMenu)) return false;
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
setKeyEventMods(Modifiers* mods, unsigned int state)
{
    if (state & ControlMask) mods->ctrl = true;
    if (state & Mod1Mask) mods->alt = true;
    if (state & Mod4Mask) mods->hyper = true;
    if (state & ShiftMask) mods->shift = true;
}

static SpecialKey
getSpecialKey(KeySym keysym)
{
    for (size_t i = 0; i < specialkeysLen; i++)
    {
        if (specialkeys[i].keysym == keysym) return specialkeys[i].special;
    }
    return SPECIAL_KEY_NONE;
}

static KeyType
processKey(Key* key, unsigned int state, KeySym keysym, char* buffer, int len)
{
    key->repr = buffer;
    key->len = len;
    setKeyEventMods(&key->mods, state);
    key->special = getSpecialKey(keysym);
    if (keyIsStrictlyMod(key)) return KEY_TYPE_IS_STRICTLY_MOD;
    if (keyIsSpecial(key)) return KEY_TYPE_IS_SPECIAL;
    if (keyIsNormal(key)) return KEY_TYPE_IS_NORMAL;
    return KEY_TYPE_IS_UNKNOWN;
}

static MenuStatus
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

    if (status == XLookupNone || status == XBufferOverflow) return MENU_STATUS_RUNNING;

    switch (processKey(&key, state, keysym, buffer, len))
    {
    case KEY_TYPE_IS_STRICTLY_MOD: return MENU_STATUS_RUNNING;
    case KEY_TYPE_IS_SPECIAL: /* FALLTHROUGH */
    case KEY_TYPE_IS_NORMAL: return handleKeypress(mainMenu, &key);
    case KEY_TYPE_IS_UNKNOWN:
        debugMsg(debug, "Encountered an unknown key. Most likely a modifier key press event.");
        if (debug) disassembleKey(&key);
        return MENU_STATUS_RUNNING;
    default: errorMsg("Got an unkown return value from 'processKey'."); break;
    }

    return MENU_STATUS_EXIT_SOFTWARE;
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
            if (ev.xexpose.count == 0) if (!render()) return EX_SOFTWARE;
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
            case MENU_STATUS_RUNNING: break;
            case MENU_STATUS_DAMAGED: if (!render()) return EX_SOFTWARE; break;
            case MENU_STATUS_EXIT_OK: return EX_OK;
            case MENU_STATUS_EXIT_SOFTWARE: return EX_SOFTWARE;
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
runX11(Menu* menu)
{
    assert(menu);

    int result = EX_SOFTWARE;
    checkLocale();
    mainMenu = x11.menu = menu;
    mainMenu->cleanupfp = cleanupAsync;
    mainMenu->xp = &x11;
    debug = mainMenu->debug;
    if (!initX11()) return result;
    grabkeyboard();
    if (render())
    {
        result = eventHandler();
    }
    cleanup(&x11);
    return result;
}
