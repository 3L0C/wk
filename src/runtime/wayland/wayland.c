#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sysexits.h>
#include <unistd.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

/* common includes */
#include "common/common.h"
#include "common/debug.h"
#include "common/menu.h"
#include "common/key_chord.h"

/* runtime includes */
#include "common/string.h"
#include "runtime/cairo.h"

/* local includes */
#include "wayland.h"
#include "registry.h"
#include "runtime/common.h"
#include "window.h"
#include "wlr-layer-shell-unstable-v1.h"

typedef struct
{
    SpecialKey special;
    xkb_keysym_t keysym;
} WaylandSpecialKey;

static const WaylandSpecialKey specialkeys[] = {
    { SPECIAL_KEY_NONE, XKB_KEY_NoSymbol },
    { SPECIAL_KEY_LEFT, XKB_KEY_Left },
    { SPECIAL_KEY_LEFT, XKB_KEY_KP_Left },
    { SPECIAL_KEY_RIGHT, XKB_KEY_Right },
    { SPECIAL_KEY_RIGHT, XKB_KEY_KP_Right },
    { SPECIAL_KEY_UP, XKB_KEY_Up },
    { SPECIAL_KEY_UP, XKB_KEY_KP_Up },
    { SPECIAL_KEY_DOWN, XKB_KEY_Down },
    { SPECIAL_KEY_DOWN, XKB_KEY_KP_Down },
    { SPECIAL_KEY_TAB, XKB_KEY_Tab },
    { SPECIAL_KEY_TAB, XKB_KEY_KP_Tab },
    { SPECIAL_KEY_SPACE, XKB_KEY_space },
    { SPECIAL_KEY_SPACE, XKB_KEY_KP_Space },
    { SPECIAL_KEY_RETURN, XKB_KEY_Return },
    { SPECIAL_KEY_RETURN, XKB_KEY_KP_Enter },
    { SPECIAL_KEY_DELETE, XKB_KEY_Delete },
    { SPECIAL_KEY_DELETE, XKB_KEY_KP_Delete },
    { SPECIAL_KEY_ESCAPE, XKB_KEY_Escape },
    { SPECIAL_KEY_HOME, XKB_KEY_Home },
    { SPECIAL_KEY_HOME, XKB_KEY_KP_Home },
    { SPECIAL_KEY_PAGE_UP, XKB_KEY_Page_Up },
    { SPECIAL_KEY_PAGE_UP, XKB_KEY_KP_Page_Up },
    { SPECIAL_KEY_PAGE_DOWN, XKB_KEY_Page_Down },
    { SPECIAL_KEY_PAGE_DOWN, XKB_KEY_KP_Page_Down },
    { SPECIAL_KEY_END, XKB_KEY_End },
    { SPECIAL_KEY_END, XKB_KEY_KP_End },
    { SPECIAL_KEY_BEGIN, XKB_KEY_Begin },
    { SPECIAL_KEY_BEGIN, XKB_KEY_KP_Begin },
    { SPECIAL_KEY_F1, XKB_KEY_F1 },
    { SPECIAL_KEY_F2, XKB_KEY_F2 },
    { SPECIAL_KEY_F3, XKB_KEY_F3 },
    { SPECIAL_KEY_F4, XKB_KEY_F4 },
    { SPECIAL_KEY_F5, XKB_KEY_F5 },
    { SPECIAL_KEY_F6, XKB_KEY_F6 },
    { SPECIAL_KEY_F7, XKB_KEY_F7 },
    { SPECIAL_KEY_F8, XKB_KEY_F8 },
    { SPECIAL_KEY_F9, XKB_KEY_F9 },
    { SPECIAL_KEY_F10, XKB_KEY_F10 },
    { SPECIAL_KEY_F11, XKB_KEY_F11 },
    { SPECIAL_KEY_F12, XKB_KEY_F12 },
    { SPECIAL_KEY_F13, XKB_KEY_F13 },
    { SPECIAL_KEY_F14, XKB_KEY_F14 },
    { SPECIAL_KEY_F15, XKB_KEY_F15 },
    { SPECIAL_KEY_F16, XKB_KEY_F16 },
    { SPECIAL_KEY_F17, XKB_KEY_F17 },
    { SPECIAL_KEY_F18, XKB_KEY_F18 },
    { SPECIAL_KEY_F19, XKB_KEY_F19 },
    { SPECIAL_KEY_F20, XKB_KEY_F20 },
    { SPECIAL_KEY_F21, XKB_KEY_F21 },
    { SPECIAL_KEY_F22, XKB_KEY_F22 },
    { SPECIAL_KEY_F23, XKB_KEY_F23 },
    { SPECIAL_KEY_F24, XKB_KEY_F24 },
    { SPECIAL_KEY_F25, XKB_KEY_F25 },
    { SPECIAL_KEY_F26, XKB_KEY_F26 },
    { SPECIAL_KEY_F27, XKB_KEY_F27 },
    { SPECIAL_KEY_F28, XKB_KEY_F28 },
    { SPECIAL_KEY_F29, XKB_KEY_F29 },
    { SPECIAL_KEY_F30, XKB_KEY_F30 },
    { SPECIAL_KEY_F31, XKB_KEY_F31 },
    { SPECIAL_KEY_F32, XKB_KEY_F32 },
    { SPECIAL_KEY_F33, XKB_KEY_F33 },
    { SPECIAL_KEY_F34, XKB_KEY_F34 },
    { SPECIAL_KEY_F35, XKB_KEY_F35 },
    /* XF86 keys */
    { SPECIAL_KEY_AUDIO_VOL_DOWN, XKB_KEY_XF86AudioLowerVolume },
    { SPECIAL_KEY_AUDIO_VOL_MUTE, XKB_KEY_XF86AudioMute },
    { SPECIAL_KEY_AUDIO_VOL_UP, XKB_KEY_XF86AudioRaiseVolume },
    { SPECIAL_KEY_AUDIO_PLAY, XKB_KEY_XF86AudioPlay },
    { SPECIAL_KEY_AUDIO_STOP, XKB_KEY_XF86AudioStop },
    { SPECIAL_KEY_AUDIO_PREV, XKB_KEY_XF86AudioPrev },
    { SPECIAL_KEY_AUDIO_NEXT, XKB_KEY_XF86AudioNext },
};

static const size_t specialkeysLen = sizeof(specialkeys) / sizeof(specialkeys[0]);

static int efd;
static bool debug = false;

static void
renderWindowsIfPending(Menu* menu, Wayland* wayland)
{
    assert(menu), assert(wayland);

    WaylandWindow* window;
    wl_list_for_each(window, &wayland->windows, link)
    {
        if (window->renderPending) windowRender(window, wayland->display, menu);
    }
    wl_display_flush(wayland->display);
}

static void
scheduleWindowsRenderIfDirty(Menu* menu, Wayland* wayland)
{
    assert(menu), assert(wayland);

    WaylandWindow* window;
    wl_list_for_each(window, &wayland->windows, link)
    {
        if (window->renderPending)
        {
            renderWindowsIfPending(menu, wayland);
        }
        if (menu->dirty) windowScheduleRender(window);
    }

    menu->dirty = false;
}

static bool
checkEvents(Wayland* wayland, int wait)
{
    assert(wayland);

    wl_display_dispatch_pending(wayland->display);

    if (wl_display_flush(wayland->display) < 0 && errno != EAGAIN) return false;

    struct epoll_event ep[16];
    int num = epoll_wait(efd, ep, 16, wait);
    if (num < 0) return false;

    for (int i = 0; i < num; i++)
    {
        if (ep[i].data.ptr == &wayland->fds.display)
        {
            if (ep[i].events & EPOLLERR || ep[i].events & EPOLLHUP ||
                ((ep[i].events & EPOLLIN) && wl_display_dispatch(wayland->display) < 0))
            {
                return false;
            }
        }
        else if (ep[i].data.ptr == &wayland->fds.repeat)
        {
            waylandRepeat(wayland);
        }
    }

    if (wait == 0) usleep(1000);

    return true;
}

static bool
render(Menu* menu, Wayland* wayland)
{
    assert(menu), assert(wayland);

    scheduleWindowsRenderIfDirty(menu, wayland);
    if (!checkEvents(wayland, menuIsDelayed(menu) ? 0 : -1)) return false;
    renderWindowsIfPending(menu, wayland);

    return true;
}

static void
xkbStateUpdateMask(
    Xkb* xkb,
    uint32_t depressedMods,
    uint32_t latchedMods,
    uint32_t lockedMods,
    uint32_t depressedLayout,
    uint32_t lockedLayout,
    uint32_t group)
{
    assert(xkb);

    xkb_state_update_mask(
        xkb->state, depressedMods, latchedMods, lockedMods, depressedLayout, lockedLayout, group
    );
}

static void
xkbStateRemoveMask(Xkb* xkb, xkb_mod_mask_t mask)
{
    assert(xkb);

    xkbStateUpdateMask(
        xkb,
        xkb->depressedMods & ~(mask),
        xkb->latchedMods & ~(mask),
        xkb->lockedMods & ~(mask),
        0, 0, xkb->group
    );
}

static void
xkbStateRestoreMask(Xkb* xkb)
{
    assert(xkb);

    xkbStateUpdateMask(xkb, xkb->depressedMods, xkb->latchedMods, xkb->lockedMods, 0, 0, xkb->group);
}

static void
setKeyMods(Key* key, uint32_t state)
{
    assert(key);

    if (state & MOD_CTRL) key->mods |= MOD_CTRL;
    if (state & XKB_MOD_ALT) key->mods |= MOD_META;
    if (state & XKB_MOD_LOGO) key->mods |= MOD_HYPER;
    if (state & MOD_SHIFT) key->mods |= MOD_SHIFT;
}

static SpecialKey
getSpecialKey(xkb_keysym_t keysym)
{
    for (size_t i = 0; i < specialkeysLen; i++)
    {
        if (specialkeys[i].keysym == keysym) return specialkeys[i].special;
    }
    return SPECIAL_KEY_NONE;
}

static bool
xkbIsModifierKey(xkb_keysym_t keysym)
{
    return (
        keysym == XKB_KEY_Shift_L || keysym == XKB_KEY_Shift_R ||
        keysym == XKB_KEY_Control_L || keysym == XKB_KEY_Control_R ||
        keysym == XKB_KEY_Caps_Lock || keysym == XKB_KEY_Shift_Lock ||
        keysym == XKB_KEY_Meta_L || keysym == XKB_KEY_Meta_R ||
        keysym == XKB_KEY_Alt_L || keysym == XKB_KEY_Alt_R ||
        keysym == XKB_KEY_Super_L || keysym == XKB_KEY_Super_R ||
        keysym == XKB_KEY_Hyper_L || keysym == XKB_KEY_Hyper_R
    );
}

static size_t
maskedKeyGetUtf8(
    Xkb* xkb,
    uint32_t keycode,
    char* buffer,
    size_t size,
    xkb_keysym_t* keysym,
    xkb_mod_mask_t mask
) {
    assert(xkb), assert(buffer), assert(keysym);

    size_t len = 0;

    xkbStateRemoveMask(xkb, mask);
    *keysym = xkb_state_key_get_one_sym(xkb->state, keycode);
    if (!xkbIsModifierKey(*keysym))
    {
        len = xkb_state_key_get_utf8(xkb->state, keycode, buffer, size);
    }
    xkbStateRestoreMask(xkb);

    return len;
}

static bool
shiftIsSignificant(const char* a, size_t aLen, const char* b, size_t bLen)
{
    assert(a), assert(b);

    return (
        aLen != bLen ||
        memcmp(a, b, (aLen < bLen) ? aLen : bLen)
    );
}

static void
handleMysteryKeypress(Menu* menu, Key* key, xkb_keysym_t keysym, char* repr, size_t reprSize)
{
    assert(menu), assert(key), assert(repr);
    debugMsg(menu->debug, "Checking mystery key.");

    int len = xkb_keysym_get_name(keysym, repr, reprSize);
    if (len == 0)
    {
        warnMsg("[WAYLAND]: Could not get keysym.");
        return;
    }
    if (len < 0)
    {
        errorMsg("[WAYLAND]: Invalid keysym.");
        return;
    }

    stringAppendCString(&key->repr, repr);
}

static void
setKeyRepr(
    Wayland* wayland,
    Menu* menu,
    Key* key,
    xkb_keysym_t* outKeysym,
    char* repr,
    size_t reprSize
) {
    assert(wayland), assert(menu), assert(key), assert(outKeysym), assert(repr);

    Xkb* xkb = &wayland->input.xkb;
    uint32_t keycode = wayland->input.code;
    uint32_t state = wayland->input.modifiers;
    xkb_keysym_t aKeysym;
    xkb_keysym_t bKeysym;
    size_t reprLen = 0;

    char aBuffer[128] = {0};
    size_t aLen = maskedKeyGetUtf8(
        xkb,
        keycode,
        aBuffer,
        sizeof(aBuffer),
        &aKeysym,
        ~(xkb->masks[MASK_CTRL])
    );

    char bBuffer[128] = {0};
    size_t bLen = maskedKeyGetUtf8(
        xkb,
        keycode,
        bBuffer,
        sizeof(bBuffer),
        &bKeysym,
        ~(xkb->masks[MASK_SHIFT] | xkb->masks[MASK_CTRL])
    );

    if (shiftIsSignificant(aBuffer, aLen, bBuffer, bLen))
    {
        reprLen = aLen;
        *outKeysym = aKeysym;
        if (reprLen > 0) mempcpy(repr, aBuffer, reprLen);
        state &= ~(MOD_SHIFT);
    }
    else
    {
        reprLen = bLen;
        *outKeysym = bKeysym;
        if (reprLen > 0) mempcpy(repr, bBuffer, reprLen);
    }

    key->special = getSpecialKey(*outKeysym);
    if (reprLen > 0 && isNormalKey(repr, reprLen))
    {
        stringAppend(&key->repr, repr, reprLen);
    }
    else if (key->special != SPECIAL_KEY_NONE)
    {
        stringAppendCString(&key->repr, specialKeyGetRepr(key->special));
    }
    else
    {
        handleMysteryKeypress(menu, key, *outKeysym, repr, reprSize);
    }
    setKeyMods(key, state);
}

static Key
makeKeyFromEvent(Wayland* wayland, Menu* menu, xkb_keysym_t* keysym, char* buffer, size_t size)
{
    assert(wayland), assert(menu), assert(keysym), assert(buffer);

    Key key = {0};
    keyInit(&key);
    setKeyRepr(wayland, menu, &key, keysym, buffer, size);

    return key;
}

static MenuStatus
pollKey(Wayland* wayland, Menu* menu)
{
    assert(wayland), assert(menu);

    if (wayland->input.keysym == XKB_KEY_NoSymbol || !wayland->input.keyPending)
    {
        wayland->input.keyPending = false;
        return MENU_STATUS_RUNNING;
    }

    wayland->input.keyPending = false;

    xkb_keysym_t keysym;
    char buffer[128] = {0};

    Key key = makeKeyFromEvent(wayland, menu, &keysym, buffer, sizeof(buffer));
    if (stringIsEmpty(&key.repr))
    {
        return MENU_STATUS_RUNNING;
    }

    MenuStatus status = menuHandleKeypress(menu, &key);

    keyFree(&key);
    return status;
}

static bool
pollPointer(Wayland* wayland)
{
    assert(wayland);

    bool result = false;
    PointerEvent* event = &wayland->input.pointerEvent;
    if (event->state & WL_POINTER_BUTTON_STATE_PRESSED ||
        event->state & WL_POINTER_BUTTON_STATE_RELEASED)
    {
        result = true;
    }

    memset(event, 0, sizeof(*event));
    return result;
}

static bool
pollTouch(Wayland* wayland)
{
    assert(wayland);

    bool result = false;
    TouchEvent* event = &wayland->input.touchEvent;
    for (size_t i = 0; i < 2; i++)
    {
        TouchPoint* point = &event->points[i];
        if (point->eventMask & TOUCH_EVENT_DOWN)
        {
            result = true;
            break;
        }
    }

    memset(event, 0, sizeof(*event));
    return result;
}

static void
destroyWindows(Wayland* wayland)
{
    assert(wayland);

    WaylandWindow* window;
    wl_list_for_each(window, &wayland->windows, link)
    {
        windowDestroy(window);
    }
    wl_list_init(&wayland->windows);
}

static void
windowUpdateOutput(WaylandWindow* window)
{
    assert(window);

    int32_t maxScale = 1;
    uint32_t minMaxHeight = 0;
    uint32_t minMaxWidth = 0;

    SurfaceOutput* surfaceOutput;
    wl_list_for_each(surfaceOutput, &window->surfaceOutputs, link)
    {
        if (surfaceOutput->output->scale > maxScale) maxScale = surfaceOutput->output->scale;
        if (minMaxHeight == 0 || surfaceOutput->output->height < minMaxHeight)
        {
            minMaxHeight = surfaceOutput->output->height;
        }
        if (minMaxWidth == 0 || surfaceOutput->output->width < minMaxWidth)
        {
            minMaxWidth = surfaceOutput->output->width;
        }
    }

    if (minMaxHeight != window->maxHeight) window->maxHeight = minMaxHeight;
    if (minMaxWidth != window->maxWidth) window->maxWidth = minMaxWidth;
    if (maxScale != window->scale) window->scale = maxScale;
}

static void
surfaceEnter(void* data, struct wl_surface* surface, struct wl_output* wlOutput)
{
    (void)surface;
    WaylandWindow* window = data;
    Wayland* wayland = window->wayland;

    Output* output;
    wl_list_for_each(output, &wayland->outputs, link)
    {
        if (output->output == wlOutput)
        {
            SurfaceOutput* surfaceOutput = calloc(1, sizeof(SurfaceOutput));
            surfaceOutput->output = output;
            wl_list_insert(&window->surfaceOutputs, &surfaceOutput->link);
            break;
        }
    }

    windowUpdateOutput(window);
}

static void
surfaceLeave(void* data, struct wl_surface* surface, struct wl_output* wlOutput)
{
    (void)surface;
    WaylandWindow* window = data;

    SurfaceOutput* surfaceOutput;
    wl_list_for_each(surfaceOutput, &window->surfaceOutputs, link)
    {
        if (surfaceOutput->output->output == wlOutput)
        {
            wl_list_remove(&surfaceOutput->link);
            break;
        }
    }

    windowUpdateOutput(window);
}

static const struct wl_surface_listener surfaceListener = {
    .enter = surfaceEnter,
    .leave = surfaceLeave,
};

static void
setOverlap(Wayland* wayland, bool overlap)
{
    assert(wayland);

    WaylandWindow* window;
    wl_list_for_each(window, &wayland->windows, link)
    {
        windowSetOverlap(window, wayland->display, overlap);
    }
}

static void
grabKeyboard(Wayland* wayland, bool grab)
{
    assert(wayland);

    WaylandWindow* window;
    wl_list_for_each(window, &wayland->windows, link)
    {
        windowGrabKeyboard(window, wayland->display, grab);
    }
}

static bool
recreateWindows(Menu* menu, Wayland* wayland)
{
    assert(wayland);

    destroyWindows(wayland);
    WaylandWindow* window = calloc(1, sizeof(WaylandWindow));
    wl_list_init(&window->surfaceOutputs);
    window->wayland = wayland;
    window->position = menu->position;

    /* TODO this should not be necessary, but Sway 1.8.1 does not trigger event
     * surface.enter before we actually need to render the first frame.
     */
    window->scale = 1;
    window->maxWidth = 640;
    window->maxHeight = 480;

    struct wl_surface* surface = wl_compositor_create_surface(wayland->compositor);
    if (!surface) goto fail;

    wl_surface_add_listener(surface, &surfaceListener, window);

    Output* output = NULL;
    if (wayland->selectedOutput)
    {
        errorMsg("Selected output.");
        output = wayland->selectedOutput;
    }

    struct wl_output* wlOutput = NULL;
    if (output) wlOutput = output->output;

    if (!windowCreate(
            window, wayland->display, wayland->shm, wlOutput, wayland->layerShell, surface, menu
        )) goto fail;

    window->render = cairoPaint;
    window->renderPending = true;
    wl_list_insert(&wayland->windows, &window->link);

    setOverlap(wayland, true);
    grabKeyboard(wayland, true);
    return true;

fail:
    free(window);
    errorMsg("Wayland window creation failed.");
    return false;
}

void
waylandFree(Wayland* wayland)
{
    assert(wayland);

    destroyWindows(wayland);
    waylandRegistryDestroy(wayland);
    xkb_context_unref(wayland->input.xkb.context);

    if (wayland->display)
    {
        epoll_ctl(efd, EPOLL_CTL_DEL, wayland->fds.repeat, NULL);
        epoll_ctl(efd, EPOLL_CTL_DEL, wayland->fds.display, NULL);
        close(wayland->fds.repeat);
        wl_display_flush(wayland->display);
        wl_display_disconnect(wayland->display);
    }
}

bool
waylandInit(Menu* menu, Wayland* wayland)
{
    assert(menu), assert(wayland);

    wl_list_init(&wayland->windows);
    wl_list_init(&wayland->outputs);

    if (!(wayland->display = wl_display_connect(NULL))) goto fail;
    if (!(wayland->input.xkb.context = xkb_context_new(XKB_CONTEXT_NO_FLAGS))) goto fail;
    if (!waylandRegistryRegister(wayland, menu)) goto fail;

    wayland->fds.display = wl_display_get_fd(wayland->display);
    wayland->fds.repeat = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    wayland->input.repeatFd = &wayland->fds.repeat;
    wayland->input.keyPending = false;

    recreateWindows(menu, wayland);

    if (!efd && (efd = epoll_create1(EPOLL_CLOEXEC)) < 0) goto fail;

    struct epoll_event ep;
    ep.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    ep.data.ptr = &wayland->fds.display;
    epoll_ctl(efd, EPOLL_CTL_ADD, wayland->fds.display, &ep);

    struct epoll_event ep2;
    ep2.events = EPOLLIN;
    ep2.data.ptr = &wayland->fds.repeat;
    epoll_ctl(efd, EPOLL_CTL_ADD, wayland->fds.repeat, &ep2);
    return true;

fail:
    waylandFree(wayland);
    return false;
}

int
waylandRun(Menu* menu)
{
    assert(menu);

    debug = menu->debug;

    Wayland wayland = {0};
    int result = EX_SOFTWARE;
    if (!waylandInit(menu, &wayland))
    {
        errorMsg("Failed to create Wayland structure.");
        return EX_SOFTWARE;
    }

    MenuStatus status = MENU_STATUS_EXIT_SOFTWARE;
    do
    {
        /* Exit on pointer and touch events */
        if (pollPointer(&wayland)) break;
        if (pollTouch(&wayland)) break;

        render(menu, &wayland);
        switch (status = pollKey(&wayland, menu))
        {
        case MENU_STATUS_RUNNING: break;
        case MENU_STATUS_DAMAGED: render(menu, &wayland); break;
        case MENU_STATUS_EXIT_OK: result = EX_OK; break;
        case MENU_STATUS_EXIT_SOFTWARE: result = EX_SOFTWARE; break;
        }
    }
    while (menuStatusIsRunning(status));

    waylandFree(&wayland);

    return result;
}
