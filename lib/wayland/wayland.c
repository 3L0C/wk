#include <assert.h>
#include <bits/time.h>
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
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include "lib/cairo.h"
#include "lib/common.h"
#include "lib/debug.h"
#include "lib/menu.h"
#include "lib/types.h"
#include "lib/util.h"

#include "wayland.h"
#include "registry.h"
#include "window.h"
#include "wlr-layer-shell-unstable-v1.h"

typedef struct
{
    WkSpecial special;
    xkb_keysym_t keysym;
} SpecialKey;

static const SpecialKey specialkeys[] = {
    { WK_SPECIAL_NONE,      XKB_KEY_NoSymbol },
    { WK_SPECIAL_LEFT,      XKB_KEY_Left },
    { WK_SPECIAL_LEFT,      XKB_KEY_KP_Left },
    { WK_SPECIAL_RIGHT,     XKB_KEY_Right },
    { WK_SPECIAL_RIGHT,     XKB_KEY_KP_Right },
    { WK_SPECIAL_UP,        XKB_KEY_Up },
    { WK_SPECIAL_UP,        XKB_KEY_KP_Up },
    { WK_SPECIAL_DOWN,      XKB_KEY_Down },
    { WK_SPECIAL_DOWN,      XKB_KEY_KP_Down },
    { WK_SPECIAL_TAB,       XKB_KEY_Tab },
    { WK_SPECIAL_TAB,       XKB_KEY_KP_Tab },
    { WK_SPECIAL_SPACE,     XKB_KEY_space },
    { WK_SPECIAL_SPACE,     XKB_KEY_KP_Space },
    { WK_SPECIAL_RETURN,    XKB_KEY_Return },
    { WK_SPECIAL_RETURN,    XKB_KEY_KP_Enter },
    { WK_SPECIAL_DELETE,    XKB_KEY_Delete },
    { WK_SPECIAL_DELETE,    XKB_KEY_KP_Delete },
    { WK_SPECIAL_ESCAPE,    XKB_KEY_Escape },
    { WK_SPECIAL_HOME,      XKB_KEY_Home },
    { WK_SPECIAL_HOME,      XKB_KEY_KP_Home },
    { WK_SPECIAL_PAGE_UP,   XKB_KEY_Page_Up },
    { WK_SPECIAL_PAGE_UP,   XKB_KEY_KP_Page_Up },
    { WK_SPECIAL_PAGE_DOWN, XKB_KEY_Page_Down },
    { WK_SPECIAL_PAGE_DOWN, XKB_KEY_KP_Page_Down },
    { WK_SPECIAL_END,       XKB_KEY_End },
    { WK_SPECIAL_END,       XKB_KEY_KP_End },
    { WK_SPECIAL_BEGIN,     XKB_KEY_Begin },
    { WK_SPECIAL_BEGIN,     XKB_KEY_KP_Begin }
};

static const size_t specialkeysLen = sizeof(specialkeys) / sizeof(specialkeys[0]);

static int efd;
static bool debug = false;

static void
renderWindowsIfPending(WkMenu* props, Wayland* wayland)
{
    debugMsg(debug, "lib/wayland/wayland.c:renderWindowsIfPending:77");
    WaylandWindow* window;
    wl_list_for_each(window, &wayland->windows, link)
    {
        if (window->renderPending) windowRender(window, wayland->display, props);
    }
    wl_display_flush(wayland->display);
}

static bool
waitForEvents(Wayland* wayland)
{
    debugMsg(debug, "lib/wayland/wayland.c:waitForEvents:89");
    wl_display_dispatch_pending(wayland->display);

    if (wl_display_flush(wayland->display) < 0 && errno != EAGAIN) return false;

    struct epoll_event ep[16];
    uint32_t num = epoll_wait(efd, ep, 16, -1);
    for (uint32_t i = 0; i < num; i++)
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

    return true;
}

static void
scheduleWindowsRenderIfDirty(WkMenu* props, Wayland* wayland)
{
    debugMsg(debug, "lib/wayland/wayland.c:scheduleWindowsRenderIfDirty:118");
    WaylandWindow* window;
    wl_list_for_each(window, &wayland->windows, link)
    {
        if (window->renderPending)
        {
            renderWindowsIfPending(props, wayland);
        }
        if (props->dirty) windowScheduleRender(window);
    }

    props->dirty = false;
}

static bool
render(WkMenu* props, Wayland* wayland)
{
    debugMsg(debug, "lib/wayland/wayland.c:render:135");
    scheduleWindowsRenderIfDirty(props, wayland);
    if (!waitForEvents(wayland)) return false;
    renderWindowsIfPending(props, wayland);

    return true;
}

static struct xkb_state*
xkbCleanState(struct xkb_state* oldState, uint32_t group)
{
    debugMsg(debug, "lib/wayland/wayland.c:xkbCleanState:146");
    // Create a new xkb_state as a copy of the original state
    struct xkb_state* newState = xkb_state_new(xkb_state_get_keymap(oldState));

    // Get the control modifier mask
    xkb_mod_mask_t ctrlMask = (1 << xkb_keymap_mod_get_index(xkb_state_get_keymap(oldState), XKB_MOD_NAME_CTRL));

    // Update the modifier mask of the new state, excluding the control modifier
    xkb_state_update_mask(newState,
                          xkb_state_serialize_mods(oldState, XKB_STATE_MODS_EFFECTIVE) & ~ctrlMask,
                          xkb_state_serialize_layout(oldState, XKB_STATE_LAYOUT_EFFECTIVE),
                          0, 0, 0, group);
    return newState;
}

static void
setKeyEventMods(WkMods* wkMods, uint32_t mods)
{
    debugMsg(debug, "lib/wayland/wayland.c:setKeyEventMods:164");
    if (mods & MOD_CTRL) wkMods->ctrl = true;
    if (mods & MOD_ALT) wkMods->alt = true;
    if (mods & MOD_LOGO) wkMods->hyper = true;
    if (mods & MOD_SHIFT) wkMods->shift = true;
}

static WkSpecial
getSpecialKey(xkb_keysym_t keysym)
{
    debugMsg(debug, "lib/wayland/wayland.c:getSpecialKey:174");
    for (size_t i = 0; i < specialkeysLen; i++)
    {
        if (specialkeys[i].keysym == keysym) return specialkeys[i].special;
    }
    return WK_SPECIAL_NONE;
}

static WkKeyType
processKey(Key* key, xkb_keysym_t keysym, uint32_t mods, const char* buffer, size_t len)
{
    debugMsg(debug, "lib/wayland/wayland.c:processKey:185");
    key->key = buffer;
    key->len = len;
    setKeyEventMods(&key->mods, mods);
    key->special = getSpecialKey(keysym);
    if (keyIsStrictlyMod(key)) return WK_KEY_IS_STRICTLY_MOD;
    if (keyIsNormal(key)) return WK_KEY_IS_NORMAL;
    if (keyIsSpecial(key)) return WK_KEY_IS_SPECIAL;
    return WK_KEY_IS_UNKNOWN;
}

static WkStatus
pollKey(WkMenu* props, Wayland* wayland)
{
    debugMsg(debug, "lib/wayland/wayland.c:pollKey:196");
    assert(props && wayland);

    if (wayland->input.keysym == XKB_KEY_NoSymbol || !wayland->input.keyPending)
    {
        wayland->input.keyPending = false;
        return WK_STATUS_RUNNING;
    }

    xkb_keysym_t keysym = wayland->input.keysym;
    uint32_t mods = wayland->input.modifiers;
    struct xkb_state* cleanState = xkbCleanState(wayland->input.xkb.state, wayland->input.xkb.group);
    char buffer[32] = {0};
    size_t size = 32;
    Key key = {0};
    size_t len = xkb_state_key_get_utf8(
        cleanState, wayland->input.code, buffer, size
    );

    /* Cleanup */
    xkb_state_unref(cleanState);
    wayland->input.keyPending = false;

    if (len > size)
    {
        errorMsg(
            "Buffer too small when polling key. Buffer size '%zu', key length '%zu'.", size, len
        );
        return WK_STATUS_EXIT_SOFTWARE;
    }

    switch (processKey(&key, keysym, mods, buffer, len))
    {
    case WK_KEY_IS_STRICTLY_MOD: return WK_STATUS_RUNNING;
    case WK_KEY_IS_SPECIAL: /* FALLTHROUGH */
    case WK_KEY_IS_NORMAL: return handleKeypress(props, &key);
    case WK_KEY_IS_UNKNOWN:
        errorMsg("Encountered an unknown key.");
        if (debug) debugKey(&key);
        return WK_STATUS_EXIT_SOFTWARE;
    default: errorMsg("Got an unkown return value from 'processKey'."); break;
    }

    return WK_STATUS_EXIT_SOFTWARE;
}

static bool
pollPointer(Wayland* wayland)
{
    assert(wayland);

    debugMsg(debug, "lib/wayland/wayland.c:pollPointer:239");

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

static void
destroyWindows(Wayland* wayland)
{
    debugMsg(debug, "lib/wayland/wayland.c:destroyWindows:256");
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
    debugMsg(debug, "lib/wayland/wayland.c:windowUpdateOutput:268");
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
    debugMsg(debug, "lib/wayland/wayland.c:surfaceEnter:295");
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
    debugMsg(debug, "lib/wayland/wayland.c:surfaceLeave:318");
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
    debugMsg(debug, "lib/wayland/wayland.c:setOverlap:343");
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
    debugMsg(debug, "lib/wayland/wayland.c:grabKeyboard:356");
    assert(wayland);

    WaylandWindow* window;
    wl_list_for_each(window, &wayland->windows, link)
    {
        windowGrabKeyboard(window, wayland->display, grab);
    }
}

static bool
recreateWindows(WkMenu* props, Wayland* wayland)
{
    debugMsg(debug, "lib/wayland/wayland.c:recreateWindows:369");
    assert(wayland);

    destroyWindows(wayland);
    WaylandWindow* window = calloc(1, sizeof(WaylandWindow));
    wl_list_init(&window->surfaceOutputs);
    window->wayland = wayland;
    window->position = props->position;

    /* TODO this should not be necessary, but Sway 1.8.1 does not trigger event
     * surface.enter before we actually need to render the first frame.
     */
    window->scale = 1;
    window->maxHeight = 640;

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
            window, wayland->display, wayland->shm, wlOutput, wayland->layerShell, surface, props
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
freeWayland(Wayland* wayland)
{
    debugMsg(debug, "lib/wayland/wayland.c:freeWayland:448");
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
initWayland(WkMenu* props, Wayland* wayland)
{
    debugMsg(debug, "lib/wayland/wayland.c:initWayland:466");
    assert(props && wayland);

    wl_list_init(&wayland->windows);
    wl_list_init(&wayland->outputs);

    if (!(wayland->display = wl_display_connect(NULL))) goto fail;
    if (!(wayland->input.xkb.context = xkb_context_new(XKB_CONTEXT_NO_FLAGS))) goto fail;
    if (!waylandRegistryRegister(wayland, props)) goto fail;

    wayland->fds.display = wl_display_get_fd(wayland->display);
    wayland->fds.repeat = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    wayland->input.repeatFd = &wayland->fds.repeat;
    wayland->input.keyPending = false;

    recreateWindows(props, wayland);

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
    freeWayland(wayland);
    return false;
}

int
runWayland(WkMenu* props)
{
    assert(props);

    debug = props->debug;

    debugMsg(debug, "lib/wayland/wayland.c:runWayland:508");
    Wayland wayland = {0};
    int result = EX_SOFTWARE;
    if (!initWayland(props, &wayland))
    {
        errorMsg("Failed to create Wayland structure.");
        return EX_SOFTWARE;
    }
    debugMsg(props->debug, "Successfully created Wayland structure.");
    WkStatus status = WK_STATUS_EXIT_SOFTWARE;
    do
    {
        render(props, &wayland);
        switch (status = pollKey(props, &wayland))
        {
        case WK_STATUS_RUNNING: break;
        case WK_STATUS_DAMAGED: recreateWindows(props, &wayland); break;
        case WK_STATUS_EXIT_OK: result = EX_OK; break;
        case WK_STATUS_EXIT_SOFTWARE: result = EX_SOFTWARE; break;
        }

        /* Exit on pointer events */
        if (pollPointer(&wayland)) break;

        debugStatus(status);
    }
    while (statusIsRunning(status));

    freeWayland(&wayland);

    return result;
}
