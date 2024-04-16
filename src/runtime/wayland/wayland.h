#ifndef WK_WAYLAND_WAYLAND_H_
#define WK_WAYLAND_WAYLAND_H_

#include <stdint.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

/* common includes */
#include "common/menu.h"

/* local includes */
#include "registry.h"

typedef enum
{
    POINTER_EVENT_ENTER         = 1 << 0,
    POINTER_EVENT_LEAVE         = 1 << 1,
    POINTER_EVENT_MOTION        = 1 << 2,
    POINTER_EVENT_BUTTON        = 1 << 3,
    POINTER_EVENT_AXIS          = 1 << 4,
    POINTER_EVENT_AXIS_SOURCE   = 1 << 5,
    POINTER_EVENT_AXIS_STOP     = 1 << 6,
    POINTER_EVENT_AXIS_DISCRETE = 1 << 7,
} PointerEventMask;

typedef struct
{
    struct xkb_state* state;
    struct xkb_context* context;
    struct xkb_keymap* keymap;
    xkb_mod_mask_t masks[MASK_LAST];
    uint32_t group;
} Xkb;

typedef struct
{
    uint32_t eventMask;
    wl_fixed_t surfaceX;
    wl_fixed_t surfaceY;
    uint32_t button;
    uint32_t state;
    uint32_t time;
    uint32_t serial;
    struct
    {
        bool valid;
        wl_fixed_t value;
        int32_t discrete;
    } axes[2];
    uint32_t axisSource;
} PointerEvent;

typedef struct
{
    int* repeatFd;

    struct wl_seat* seat;
    struct wl_keyboard* keyboard;
    struct wl_pointer* pointer;
    struct wl_touch* touch;
    PointerEvent pointerEvent;
    Xkb xkb;

    xkb_keysym_t keysym;
    uint32_t code;
    uint32_t modifiers;

    xkb_keysym_t repeatKeysym;
    uint32_t repeatKey;

    struct
    {
        int32_t sec;
        int32_t nsec;
    } repeatRate;

    struct
    {
        int32_t sec;
        int32_t nsec;
    } repeatDelay;

    struct
    {
        void (*key)(enum wl_keyboard_key_state state, xkb_keysym_t keysym, uint32_t code);
    } notify;

    bool keyPending;
} Input;

typedef struct
{
    struct wl_output* output;
    struct wl_list link;
    uint32_t width;
    uint32_t height;
    int scale;
    char* name;
} Output;

typedef struct
{
    Output* output;
    struct wl_list link;
} SurfaceOutput;

typedef struct Wayland
{
    struct
    {
        int32_t display;
        int32_t repeat;
    } fds;

    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_compositor* compositor;
    struct wl_list outputs;
    Output* selectedOutput;
    struct wl_seat* seat;
    struct zwlr_layer_shell_v1* layerShell;
    struct wl_shm* shm;
    Input input;
    struct wl_list windows;
    uint32_t formats;
} Wayland;

void freeWayland(Wayland* wayland);
bool initWayland(Menu* props, Wayland* wayland);
int  runWayland(Menu* props);

#endif /* WK_WAYLAND_WAYLAND_H_ */
