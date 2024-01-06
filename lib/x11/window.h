#ifndef WK_X11_WINDOW_H_
#define WK_X11_WINDOW_H_

#include <X11/Xlib.h>

#include "lib/cairo.h"
#include "lib/common.h"
#include "lib/window.h"

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
    uint32_t maxHeight;
    uint32_t origWidth;
    uint32_t origX;
    uint32_t hmarginSize;
    float widthFactor;
    uint32_t displayed;
    int32_t monitor;
    /* align */
    int32_t yOffset;
    void (*render)(Cairo* cairo, uint32_t width, uint32_t max_height, WkProperties* props, CairoPaintResult* result);
} WkX11Window;

typedef struct
{
    Display* dispaly;
    WkX11Window window;
    WkProperties* props;
} X11;

/* struct window { */
/*     Display *display; */
/*     int32_t screen; */
/*     Drawable drawable; */
/*     XIM xim; */
/*     XIC xic; */
/*     Visual *visual; */

/*     KeySym keysym; */
/*     uint32_t mods; */

/*     struct buffer buffer; */
/*     uint32_t x, y, width, height, max_height; */
/*     uint32_t orig_width, orig_x; */
/*     uint32_t hmargin_size; */
/*     float width_factor; */
/*     uint32_t displayed; */

/*     int32_t monitor; */
/*     enum bm_align align; */
/*     int32_t y_offset; */

/*     struct { */
/*         void (*render)(struct cairo *cairo, uint32_t width, uint32_t max_height, struct bm_menu *menu, struct cairo_paint_result *result); */
/*     } notify; */
/* }; */

int runX11(WkProperties* props);

#endif /* WK_X11_WINDOW_H_ */
