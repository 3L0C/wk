#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <string.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon-names.h>
#include <xkbcommon/xkbcommon.h>

/* common includes */
#include "common/common.h"
#include "common/menu.h"

/* local includes */
#include "registry.h"
#include "wayland.h"
#include "wlr-layer-shell-unstable-v1.h"

const char* WK_XKB_MASK_NAMES[MASK_LAST] = {
    XKB_MOD_NAME_SHIFT,
    XKB_MOD_NAME_CAPS,
    XKB_MOD_NAME_CTRL,
    XKB_MOD_NAME_ALT,
    "Mod2",
    "Mod3",
    XKB_MOD_NAME_LOGO,
    "Mod5",
};

const XkbModBit WK_XKB_MODS[MASK_LAST] = {
    MOD_SHIFT,
    MOD_CAPS,
    MOD_CTRL,
    MOD_ALT,
    MOD_MOD2,
    MOD_MOD3,
    MOD_LOGO,
    MOD_MOD5,
};

static bool debug = false;

static void
shmFormat(void* data, struct wl_shm* wl_shm, uint32_t format)
{
    (void)wl_shm;
    Wayland* wayland = data;
    wayland->formats |= (1 << format);
}

struct wl_shm_listener shmListener = {
    .format = shmFormat,
};

static void
keyboardHandleKeymap(void* data, struct wl_keyboard* keyboard, uint32_t format, int fd, uint32_t size)
{
    (void)keyboard;
    Input* input = data;

    if (!data) goto exit;

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) goto exit;

    char* mapstr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapstr == MAP_FAILED) goto exit;

    struct xkb_keymap* keymap = xkb_keymap_new_from_string(
        input->xkb.context, mapstr, XKB_KEYMAP_FORMAT_TEXT_V1, 0
    );
    munmap(mapstr, size);
    close(fd);

    if (!keymap)
    {
        errorMsg("Failed to compile keymap.");
        return;
    }

    struct xkb_state* state = xkb_state_new(keymap);
    if (!state)
    {
        errorMsg("Failed to create XKB state.");
        xkb_keymap_unref(keymap);
        return;
    }

    xkb_keymap_unref(input->xkb.keymap);
    xkb_state_unref(input->xkb.state);
    input->xkb.keymap = keymap;
    input->xkb.state = state;

    for (uint32_t i = 0; i < MASK_LAST; i++)
    {
        input->xkb.masks[i] = 1 << xkb_keymap_mod_get_index(input->xkb.keymap, WK_XKB_MASK_NAMES[i]);
    }

    return;

exit:
    close(fd);
    return;
}

static void
keyboardHandleEnter(
    void* data,
    struct wl_keyboard* keyboard,
    uint32_t serial,
    struct wl_surface* surface,
    struct wl_array* keys)
{
    (void)data, (void)keyboard, (void)serial, (void)surface, (void)keys;
}

static void
keyboardHandleLeave(
    void* data,
    struct wl_keyboard* keyboard,
    uint32_t serial,
    struct wl_surface* surface)
{
    (void)keyboard, (void)serial, (void)surface;
    Input* input = data;
    struct itimerspec its = {
        .it_interval.tv_sec = 0,
        .it_interval.tv_nsec = 0,
        .it_value.tv_sec = 0,
        .it_value.tv_nsec = 0
    };
    timerfd_settime(*input->repeatFd, 0, &its, NULL);
}

static void
press(Input* input, xkb_keysym_t keysym, uint32_t key, enum wl_keyboard_key_state state)
{
    if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
    {
        input->keysym = keysym;
        input->code = key + 8;
        input->keyPending = true;
    }
    else if (!input->keyPending)
    {
        input->keysym = XKB_KEY_NoSymbol;
        input->code = 0;
    }

    if (input->notify.key) input->notify.key(state, keysym, key);
}

static void
keyboardHandleKey(
    void* data,
    struct wl_keyboard* keyboard,
    uint32_t serial,
    uint32_t time,
    uint32_t key,
    uint32_t stateW)
{
    (void)keyboard, (void)serial, (void)time;
    Input* input = data;
    enum wl_keyboard_key_state state = stateW;

    if (!input->xkb.state) return;

    xkb_keysym_t keysym = xkb_state_key_get_one_sym(input->xkb.state, key + 8);
    press(input, keysym, key, state);

    if (state == WL_KEYBOARD_KEY_STATE_PRESSED &&
        xkb_keymap_key_repeats(input->xkb.keymap, input->code))
    {
        struct itimerspec its = {
            .it_interval.tv_sec = input->repeatRate.sec,
            .it_interval.tv_nsec = input->repeatRate.nsec,
            .it_value.tv_sec = input->repeatDelay.sec,
            .it_value.tv_nsec = input->repeatDelay.nsec,
        };
        input->repeatKeysym = keysym;
        input->repeatKey = key;
        timerfd_settime(*input->repeatFd, 0, &its, NULL);
    }
    else if (state == WL_KEYBOARD_KEY_STATE_RELEASED && key == input->repeatKey)
    {
        struct itimerspec its = {
            .it_interval.tv_sec = 0,
            .it_interval.tv_nsec = 0,
            .it_value.tv_sec = 0,
            .it_value.tv_nsec = 0
        };
        timerfd_settime(*input->repeatFd, 0, &its, NULL);
    }
}

static void
keyboardHandleModifiers(
    void* data,
    struct wl_keyboard* keyboard,
    uint32_t serial,
    uint32_t depressedMods,
    uint32_t latchedMods,
    uint32_t lockedMods,
    uint32_t group)
{
    (void)keyboard, (void)serial;
    Input* input = data;
    input->xkb.group = group;
    input->xkb.depressedMods = depressedMods;
    input->xkb.latchedMods = depressedMods;
    input->xkb.lockedMods = lockedMods;

    if (!input->xkb.keymap) return;

    xkb_state_update_mask(input->xkb.state, depressedMods, latchedMods, lockedMods, 0, 0, group);

    xkb_mod_mask_t mask = xkb_state_serialize_mods(
        input->xkb.state, XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED
    );

    input->modifiers = 0;
    for (uint32_t i = 0; i < MASK_LAST; i++)
    {
        if (mask & input->xkb.masks[i]) input->modifiers |= WK_XKB_MODS[i];
    }
}

static void
setRepeatInfo(Input* input, int32_t rate, int32_t delay)
{
    assert(input);

    input->repeatRate.sec = input->repeatRate.nsec = 0;
    input->repeatDelay.sec = input->repeatDelay.nsec = 0;

    /* a rate of zero disables any repeating, regardless of the delay's value */
    if (rate == 0) return;

    if (rate == 1)
    {
        input->repeatRate.sec = 1;
    }
    else
    {
        input->repeatRate.nsec = 1000000000 / rate;
    }

    input->repeatDelay.sec = delay / 1000;
    delay -= (input->repeatDelay.sec * 1000);
    input->repeatDelay.nsec = delay * 1000 * 1000;
}

static void
keyboardHandleRepeatInfo(void *data, struct wl_keyboard* keyboard, int32_t rate, int32_t delay)
{
    (void)keyboard;
    setRepeatInfo(data, rate, delay);
}

static void
pointerHandleAxis(
    void* data,
    struct wl_pointer* pointer,
    uint32_t time,
    uint32_t axis,
    wl_fixed_t value)
{
    (void)data, (void)pointer, (void)time, (void)axis, (void)value;
}

static void
pointerHandleAxisSource(void* data, struct wl_pointer* pointer, uint32_t axisSource)
{
    (void)pointer, (void)pointer, (void)axisSource;
}

static void
pointerHandleAxisStop(void* data, struct wl_pointer* pointer, uint32_t time, uint32_t axis)
{
    (void)data, (void)pointer, (void)time, (void)axis;
}

static void
pointerHandleAxisDiscrete(void* data, struct wl_pointer* pointer, uint32_t axis, int32_t discrete)
{
    (void)data, (void)pointer, (void)axis, (void)discrete;
}

static void
pointerHandleAxisRelativeDirection(void* data, struct wl_pointer* pointer, uint32_t a, uint32_t b)
{
    (void)data, (void)pointer, (void)a, (void)b;
}

static void
pointerHandleAxisValue120(void* data, struct wl_pointer* pointer, uint32_t a, int32_t b)
{
    (void)data, (void)pointer, (void)a, (void)b;
}

static void
pointerHandleButton(
    void* data,
    struct wl_pointer* pointer,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state)
{
    (void)pointer;
    Input* input = data;
    input->pointerEvent.eventMask |= POINTER_EVENT_BUTTON;
    input->pointerEvent.time = time;
    input->pointerEvent.serial = serial;
    input->pointerEvent.button = button;
    input->pointerEvent.state |= state;
}

static void
pointerHandleEnter(
    void* data,
    struct wl_pointer* pointer,
    uint32_t serial,
    struct wl_surface* surface,
    wl_fixed_t surfaceX,
    wl_fixed_t surfaceY)
{
    (void)data, (void)pointer, (void)serial, (void)surface, (void)surfaceX, (void)surfaceY;
}

static void
pointerHandleFrame(void* data, struct wl_pointer* pointer)
{
    (void)data, (void)pointer;
}

static void
pointerHandleLeave(
    void* data,
    struct wl_pointer* pointer,
    uint32_t serial,
    struct wl_surface* surface)
{
    (void)data, (void)pointer, (void)serial, (void)surface;
}

static void
pointerHandleMotion(
    void* data,
    struct wl_pointer* pointer,
    uint32_t time,
    wl_fixed_t surfaceX,
    wl_fixed_t surfaceY)
{
    (void)data, (void)pointer, (void)time, (void)surfaceX, (void)surfaceY;
}

static void
touchHandleCancel(void* data, struct wl_touch* touch)
{
    (void)data, (void)touch;
}

static TouchPoint *
getTouchPoint(Input *input, int32_t id)
{
    TouchEvent *touch = &input->touchEvent;
    int invalid = -1;
    for (size_t i = 0; i < 2; ++i)
    {
        if (touch->points[i].id == id) invalid = i;
        if (invalid == -1 && !touch->points[i].valid) invalid = i;
    }
    if (invalid == -1) return NULL;
    touch->points[invalid].id = id;
    return &touch->points[invalid];
}

static void
resetAllStartPosition(Input* input)
{
    TouchEvent* event = &input->touchEvent;
    for (size_t i = 0; i < 2; ++i)
    {
        TouchPoint* point = &event->points[i];

        if (!point->valid) continue;

        point->surfaceStartX = point->surfaceX;
        point->surfaceStartY = point->surfaceY;
    }
}

static void
revalidateAllReleased(Input* input)
{
    TouchEvent* event = &input->touchEvent;
    for (size_t i = 0; i < 2; ++i)
    {
        TouchPoint* point = &event->points[i];

        if (!point->valid && point->eventMask & TOUCH_EVENT_DOWN) point->valid = true;
    }
}

static void
touchHandleDown(
    void* data,
    struct wl_touch* touch,
    uint32_t serial,
    uint32_t time,
    struct wl_surface* surface,
    int32_t id,
    wl_fixed_t x,
    wl_fixed_t y)
{
    (void) touch, (void) surface;
    Input* input = data;
    TouchPoint* point = getTouchPoint(input, id);
    if (point == NULL) return;

    point->valid = true;
    point->eventMask = TOUCH_EVENT_DOWN;
    point->surfaceX = x,
    point->surfaceY = y;
    input->touchEvent.time = time;
    input->touchEvent.serial = serial;
    input->touchEvent.active += 1;

    revalidateAllReleased(input);
    resetAllStartPosition(input);
}

static void
touchHandleFrame(void* data, struct wl_touch* touch)
{
    (void)data, (void)touch;
}

static void
touchHandleMotion(
    void* data,
    struct wl_touch* touch,
    uint32_t time,
    int32_t id,
    wl_fixed_t x,
    wl_fixed_t y)
{
    (void)data, (void)touch, (void)time, (void)id, (void)x, (void)y;
}

static void
touchHandleOrientation(void* data, struct wl_touch* touch, int32_t id, wl_fixed_t orientation)
{
    (void)data, (void)touch, (void)id, (void)orientation;
}

static void
touchHandleShape(void* data, struct wl_touch* touch, int32_t id, wl_fixed_t major, wl_fixed_t minor)
{
    (void)data, (void)touch, (void)id, (void)major, (void)minor;
}

static void
touchHandleUp(void* data, struct wl_touch* touch, uint32_t serial, uint32_t time, int32_t id)
{
    (void) time, (void) touch, (void) serial;
    Input* input = data;
    TouchPoint* point = getTouchPoint(input, id);
    if (point == NULL) return;

    point->eventMask |= TOUCH_EVENT_UP;
    input->touchEvent.active -= 1;

    resetAllStartPosition(input);
}

static const struct wl_pointer_listener pointerListener = {
    .axis = pointerHandleAxis,
    .axis_discrete = pointerHandleAxisDiscrete,
    .axis_relative_direction = pointerHandleAxisRelativeDirection,
    .axis_source = pointerHandleAxisSource,
    .axis_stop = pointerHandleAxisStop,
    .axis_value120 = pointerHandleAxisValue120,
    .button = pointerHandleButton,
    .enter = pointerHandleEnter,
    .frame = pointerHandleFrame,
    .leave = pointerHandleLeave,
    .motion = pointerHandleMotion,
};

static const struct wl_keyboard_listener keyboardListener = {
    .keymap = keyboardHandleKeymap,
    .enter = keyboardHandleEnter,
    .leave = keyboardHandleLeave,
    .key = keyboardHandleKey,
    .modifiers = keyboardHandleModifiers,
    .repeat_info = keyboardHandleRepeatInfo,
};

static const struct wl_touch_listener touchListener = {
    .cancel = touchHandleCancel,
    .down = touchHandleDown,
    .frame = touchHandleFrame,
    .motion = touchHandleMotion,
    .orientation = touchHandleOrientation,
    .shape = touchHandleShape,
    .up = touchHandleUp,
};

static void
seatHandleCapabilities(void* data, struct wl_seat* seat, enum wl_seat_capability caps)
{
    Input* input = data;

    if (!input->seat) input->seat = seat;

    if (caps & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        input->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(input->keyboard, &keyboardListener, data);
    }

    if (caps & WL_SEAT_CAPABILITY_POINTER)
    {
        input->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(input->pointer, &pointerListener, data);
    }

    if (caps & WL_SEAT_CAPABILITY_TOUCH)
    {
        input->touch = wl_seat_get_touch(seat);
        wl_touch_add_listener(input->touch, &touchListener, data);
    }

    if (seat == input->seat &&
        !(caps & WL_SEAT_CAPABILITY_KEYBOARD) &&
        !(caps & WL_SEAT_CAPABILITY_POINTER))
    {
        wl_keyboard_destroy(input->keyboard);
        input->seat = NULL;
        input->keyboard = NULL;
        input->pointer = NULL;
        input->touch = NULL;
    }
}

static void
seatHandleName(void* data, struct wl_seat* seat, const char* name)
{
    (void)data, (void)seat, (void)name;
}

static struct wl_seat_listener seatListener = {
    .capabilities = seatHandleCapabilities,
    .name = seatHandleName,
};

static void
displayHandleDescription(void* data, struct wl_output* wlOut, const char* description)
{
    (void)data, (void)wlOut, (void)description;
}

static void
displayHandleDone(void* data, struct wl_output* output)
{
    (void)data, (void)output;
}

static void
displayHandleGeometry(
    void* data,
    struct wl_output* output,
    int x,
    int y,
    int physicalw,
    int physicalh,
    int subpixel,
    const char* make,
    const char* model,
    int transform)
{
    (void)data, (void)output, (void)x, (void)y, (void)physicalw,
        (void)physicalh, (void)subpixel, (void)make, (void)model, (void)transform;
}

static void
displayHandleMode(
    void* data,
    struct wl_output* wlOut,
    uint32_t flags,
    int width,
    int height,
    int refresh)
{
    (void)wlOut, (void)refresh;

    Output* output = data;

    if (flags & WL_OUTPUT_MODE_CURRENT)
    {
        output->width = width;
        output->height = height;
    }
}

static void
displayHandleName(void* data, struct wl_output* wlOut, const char* name)
{
    (void)wlOut;
    Output* output = data;
    if (output->name) free(output->name);
    output->name = strdup(name);
}

static void
displayHandleScale(void* data, struct wl_output* wlOut, int32_t scale)
{
    (void)wlOut;
    Output* output = data;
    assert(scale > 0);
    output->scale = scale;
}

static const struct wl_output_listener outputListener = {
    .description = displayHandleDescription,
    .done = displayHandleDone,
    .geometry = displayHandleGeometry,
    .mode = displayHandleMode,
    .name = displayHandleName,
    .scale = displayHandleScale,
};

static void
registryHandleGlobal(
    void* data,
    struct wl_registry* registry,
    uint32_t id,
    const char* interface,
    uint32_t version)
{
    (void)version;
    Wayland* wayland = data;

    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        wayland->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 4);
    }
    else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0)
    {
        wayland->layerShell = wl_registry_bind(registry, id, &zwlr_layer_shell_v1_interface, 2);
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        wayland->seat = wl_registry_bind(registry, id, &wl_seat_interface, 7);
        wl_seat_add_listener(wayland->seat, &seatListener, &wayland->input);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        wayland->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
        wl_shm_add_listener(wayland->shm, &shmListener, data);
    }
    else if (strcmp(interface, wl_output_interface.name) == 0)
    {
        struct wl_output* wlOut = wl_registry_bind(registry, id, &wl_output_interface, 4);
        Output* output = calloc(1, sizeof(Output));
        output->output = wlOut;
        wl_list_insert(&wayland->outputs, &output->link);
        wl_output_add_listener(wlOut, &outputListener, output);
    }
}

static void
registryHandleGlobalRemove(void* data, struct wl_registry* registry, uint32_t name)
{
    (void)data, (void)registry, (void)name;
}

static const struct wl_registry_listener registryListener = {
    .global = registryHandleGlobal,
    .global_remove = registryHandleGlobalRemove,
};

void
waylandRepeat(Wayland* wayland)
{
    assert(wayland);

    uint64_t exp;
    if (read(wayland->fds.repeat, &exp, sizeof(exp)) != sizeof(exp)) return;

    if (wayland->input.notify.key)
    {
        wayland->input.notify.key(
            WL_KEYBOARD_KEY_STATE_PRESSED,
            wayland->input.repeatKeysym,
            wayland->input.repeatKey + 8
        );
    }
    press(
        &wayland->input,
        wayland->input.repeatKeysym,
        wayland->input.repeatKey,
        WL_KEYBOARD_KEY_STATE_PRESSED
    );
}

void
waylandRegistryDestroy(Wayland* wayland)
{
    assert(wayland);

    if (wayland->shm) wl_shm_destroy(wayland->shm);
    if (wayland->layerShell) zwlr_layer_shell_v1_destroy(wayland->layerShell);
    if (wayland->compositor) wl_compositor_destroy(wayland->compositor);
    if (wayland->registry) wl_registry_destroy(wayland->registry);

    xkb_keymap_unref(wayland->input.xkb.keymap);
    xkb_state_unref(wayland->input.xkb.state);
}

bool
waylandRegistryRegister(Wayland* wayland, Menu* menu)
{
    assert(wayland), assert(menu);

    debug = menu->debug;
    if (!(wayland->registry = wl_display_get_registry(wayland->display))) return false;

    wl_registry_add_listener(wayland->registry, &registryListener, wayland);
    wl_display_roundtrip(wayland->display); /* trip 1, registry globals */
    if (!wayland->compositor || !wayland->seat || !wayland->shm || !wayland->layerShell) return false;

    wl_display_roundtrip(wayland->display); /* trip 1, registry globals */
    if (!wayland->input.keyboard || !(wayland->formats & (1 << WL_SHM_FORMAT_ARGB8888))) return false;

    setRepeatInfo(&wayland->input, 40, 400);
    return true;
}
