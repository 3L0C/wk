#ifndef WK_X11_WINDOW_H_
#define WK_X11_WINDOW_H_

#include <X11/Xlib.h>

#include "lib/cairo.h"
#include "lib/menu.h"

typedef struct
{
    Cairo cairo;
    uint32_t width;
    uint32_t height;
    bool created;
} Buffer;

typedef struct
{
    Display* display;
    int32_t screen;
    Drawable drawable;
    XIM xim;
    XIC xic;
    Visual* visual;
    KeySym keysym;
    uint32_t mods;
    Buffer buffer;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t border;
    uint32_t maxHeight;
    float widthFactor;
    uint32_t displayed;
    int32_t monitor;
    struct display {
        uint32_t x, y, w, h;
    } root;
    CairoPaint paint;
    void (*render)(Cairo* cairo, WkMenu* menu);
} WkX11Window;

typedef struct
{
    Display* dispaly;
    WkX11Window window;
    WkMenu* menu;
} X11;

int runX11(WkMenu* menu);

#endif /* WK_X11_WINDOW_H_ */
