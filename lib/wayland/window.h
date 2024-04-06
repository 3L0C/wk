#ifndef WK_WAYLAND_WINDOW_H_
#define WK_WAYLAND_WINDOW_H_

#include <stdint.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "lib/cairo.h"
#include "lib/menu.h"

#include "wlr-layer-shell-unstable-v1.h"

typedef struct
{
    Cairo cairo;
    struct wl_buffer* buffer;
    uint32_t width;
    uint32_t height;
    bool busy;
} Buffer;

typedef struct
{
    struct Wayland* wayland;
    struct wl_list surfaceOutputs;
    struct wl_surface* surface;
    struct wl_callback* framecb;
    struct zwlr_layer_surface_v1* layerSurface;
    struct wl_shm* shm;
    Buffer buffers[2];
    CairoPaint paint;
    /* uint32_t x; */
    uint32_t windowGap;
    uint32_t width;
    uint32_t height;
    uint32_t maxWidth;
    uint32_t maxHeight;
    /* uint32_t hpadding; /\* hmargin_size in bemenu *\/ */
    /* uint32_t wpadding; /\* hmargin_size in bemenu *\/ */
    /* float widthFactor; */
    int32_t scale;
    uint32_t displayed;
    struct wl_list link;
    WkWindowPosition position;
    /* int32_t yOffset; */
    uint32_t alignAnchor;
    bool renderPending;
    void (*render)(Cairo* cairo, WkMenu* menu);
} WaylandWindow;

void windowScheduleRender(WaylandWindow* window);
bool windowRender(WaylandWindow* window, struct wl_display* display, WkMenu* menu);
void windowDestroy(WaylandWindow* window);
void windowSetWidth(WaylandWindow* window,
                    struct wl_display* display,
                    uint32_t margin,
                    float factor,
                    WkMenu* menu);
void windowSetAlign(WaylandWindow* window, struct wl_display* display, WkWindowPosition position);
void windowSetYOffset(WaylandWindow* window, struct wl_display* display, int32_t yOffset);
void windowGrabKeyboard(WaylandWindow* window, struct wl_display* display, bool grab);
void windowSetOverlap(WaylandWindow* window, struct wl_display* display, bool overlap);
bool windowCreate(WaylandWindow* window, struct wl_display* display, struct wl_shm* shm,
                  struct wl_output* wlOutput, struct zwlr_layer_shell_v1* layerShell,
                  struct wl_surface* surface, WkMenu* menu);

#endif /* WK_WAYLAND_WINDOW_H_ */
