#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sysexits.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

/* common includes */
#include "common/common.h"
#include "common/debug.h"
#include "common/memory.h"
#include "common/menu.h"

/* runtime includes */
#include "runtime/cairo.h"

/* local includes */
#include "wayland.h"
#include "window.h"
#include "wlr-layer-shell-unstable-v1.h"

static int
setCloexecOrClose(int fd)
{
    if (fd == -1) return -1;

    long flags = fcntl(fd, F_GETFD);
    if (flags == -1) goto error;
    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) goto error;

    return fd;

error:
    close(fd);
    return -1;
}

static int
createTmpfileCloexec(char* tmpName)
{
    assert(tmpName);

    int fd;

#ifdef HAVE_MKOSTEMP
    if ((fd = mkostemp(tmpName, O_CLOEXEC)) >= 0) unlink(tmpName);
#else
    if ((fd = mkstemp(tmpName)) >= 0)
    {
        fd = setCloexecOrClose(fd);
        unlink(tmpName);
    }
#endif

    return fd;
}

static int
osCreateAnonymousFile(off_t size)
{
    static const char template[] = "wk-shared-XXXXXX";
    int fd;
    int result;

    const char* path = getenv("XDG_RUNTIME_DIR");
    if (!path || strlen(path) <= 0)
    {
        errno = ENOENT;
        return -1;
    }

    char* ts = (path[strlen(path) - 1] == '/') ? "" : "/" ;
    size_t len = snprintf(NULL, 0, "%s%s%s", path, ts, template) + 1; /* +1 for null byte '\0' */
    char* name = ALLOCATE(char, len);
    if (!name) return -1;
    snprintf(name, len, "%s%s%s", path, ts, template);

    fd = createTmpfileCloexec(name);
    free(name);

    if (fd < 0) return -1;

#ifdef HAVE_POSIX_FOLLOCATE
    if ((result = posix_follocate(fd, 0, size)) != 0)
    {
        close(fd);
        errno = result;
        return -1;
    }
#else
    if ((result = ftruncate(fd, size)) < 0)
    {
        close(fd);
        return -1;
    }
#endif

    return fd;
}

static void
bufferRelease(void* data, struct wl_buffer* wlBuffer)
{
    (void)wlBuffer;
    Buffer* buffer = data;
    buffer->busy = false;
}

static const struct wl_buffer_listener bufferListener = {
    .release = bufferRelease,
};

static void
destroyBuffer(Buffer* buffer)
{
    if (buffer->buffer) wl_buffer_destroy(buffer->buffer);
    cairoDestroy(&buffer->cairo);
    memset(buffer, 0, sizeof(Buffer));
}

static bool
createBuffer(
    struct wl_shm* shm,
    Buffer* buffer,
    int32_t width,
    int32_t height,
    uint32_t format,
    int32_t scale,
    CairoPaint* paint)
{
    assert(shm), assert(buffer), assert(paint);

    uint32_t stride = width * 4;
    uint32_t size = stride * height;
    int fd = osCreateAnonymousFile(size);

    if (fd < 0)
    {
        errorMsg("Wayland: Creating a buffer file for %d B failed.", size);
        return false;
    }

    void* data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED)
    {
        errorMsg("Wayland: mmap failed.");
        close(fd);
        return false;
    }

    struct wl_shm_pool* pool = wl_shm_create_pool(shm, fd, size);
    if (!pool)
    {
        errorMsg("Wayland: wl_shm_create_pool failed.");
        close(fd);
        return false;
    }

    buffer->buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, format);
    if (!buffer->buffer) goto fail;

    wl_shm_pool_destroy(pool);
    pool = NULL;

    close(fd);
    fd = -1;

    wl_buffer_add_listener(buffer->buffer, &bufferListener, buffer);

    cairo_surface_t* surface = cairo_image_surface_create_for_data(
        data, CAIRO_FORMAT_ARGB32, width, height, stride
    );
    if (!surface) goto fail;

    buffer->cairo.scale = scale;

    if (!cairoCreateForSurface(&buffer->cairo, surface))
    {
        cairo_surface_destroy(surface);
        goto fail;
    }

    buffer->cairo.paint = paint;
    buffer->width = width;
    buffer->height = height;
    return true;

fail:
    if (fd > -1) close(fd);
    if (pool) wl_shm_pool_destroy(pool);
    destroyBuffer(buffer);
    return false;
}

static Buffer*
nextBuffer(WaylandWindow* window)
{
    assert(window);

    Buffer* buffer = NULL;
    for (size_t i = 0; i < 2; i++)
    {
        if (window->buffers[i].busy) continue;

        buffer = &window->buffers[i];
        break;
    }

    if (!buffer) return NULL;

    if (window->width * window->scale != buffer->width ||
        window->height * window->scale != buffer->height)
    {
        destroyBuffer(buffer);
    }

    if (!buffer->buffer &&
        !createBuffer(
            window->shm, buffer, window->width * window->scale, window->height * window->scale,
            WL_SHM_FORMAT_ARGB8888, window->scale, &window->paint
        ))
    {
        return NULL;
    }

    wl_surface_set_buffer_scale(window->surface, window->scale);

    return buffer;
}

static void
frameCallback(void* data, struct wl_callback* callback, uint32_t time)
{
    (void)time;
    WaylandWindow* window = data;
    wl_callback_destroy(callback);
    window->framecb = NULL;
    window->renderPending = true;
}

static const struct wl_callback_listener callbackListener = {
    frameCallback,
};

static uint32_t
getAlignAnchor(MenuWindowPosition position)
{
    uint32_t anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;

    if (position == MENU_WIN_POS_BOTTOM)
    {
        anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
    }
    else
    {
        anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
    }

    return anchor;
}

void
windowScheduleRender(WaylandWindow* window)
{
    assert(window);

    if (window->framecb) return;

    window->framecb = wl_surface_frame(window->surface);
    wl_callback_add_listener(window->framecb, &callbackListener, window);
    wl_surface_commit(window->surface);
}

static cairo_surface_t*
getThrowawaySurface(WaylandWindow* window)
{
    assert(window);

    return cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, window->width * window->scale, window->height * window->scale
    );
}

static void
resizeWinWidth(WaylandWindow* window, Menu* menu)
{
    assert(window), assert(menu);

    int32_t windowWidth = menu->windowWidth;
    uint32_t outputWidth = window->maxWidth;

    if (windowWidth < 0)
    {
        /* set width to half the size of the output */
        window->width = outputWidth / 2;
    }
    else if (windowWidth == 0 || (uint32_t)windowWidth > outputWidth)
    {
        /* make the window as wide as the output */
        window->width = outputWidth;
    }
    else
    {
        /* set the width to the desired user setting */
        window->width = windowWidth;
    }
}

static void
resizeWinHeight(WaylandWindow* window, Menu* menu)
{
    assert(window), assert(menu);

    /* Output* output = window->wayland->selectedOutput; */
    uint32_t outputHeight = window->maxHeight;

    if (window->height >= outputHeight)
    {
        /* set the height to the size of the output */
        window->windowGap = 0;
        window->height = outputHeight;
    }
}

static void
resizeWinGap(WaylandWindow* window, Menu* menu)
{
    assert(window), assert(menu);

    int32_t windowGap = menu->windowGap;
    uint32_t outputHeight = window->maxHeight;

    if (windowGap < 0)
    {
        /* set gap to 1/10th the size of the output */
        debugMsg(menu->debug, "Setting windowGap to 1/10th the screen height.");
        window->windowGap = (outputHeight / 10);
    }
    /* else if (windowGap == 0 || (uint32_t)windowGap > output->height) */
    else if ((uint32_t)windowGap > outputHeight)
    {
        /* make the window as large as possible */
        debugMsg(
            menu->debug,
            "Setting windowGap to maximum gap size possible: %u.",
            outputHeight - window->height
        );
        window->windowGap = outputHeight - window->height;
    }
    else
    {
        /* make the gap as large as the user wants */
        debugMsg(menu->debug, "Setting windowGap to user value: %u.", windowGap);
        window->windowGap = windowGap;
    }
}


static void
resizeWindow(WaylandWindow* window, Menu* menu)
{
    assert(window), assert(menu);

    window->height = cairoGetHeight(menu, getThrowawaySurface(window), window->maxHeight);
    resizeWinWidth(window, menu);
    resizeWinHeight(window, menu);
    resizeWinGap(window, menu);
}

static void
moveResizeWindow(WaylandWindow* window, struct wl_display* display)
{
    assert(window);

    zwlr_layer_surface_v1_set_size(
        window->layerSurface, window->width * window->scale, window->height * window->scale
    );
    zwlr_layer_surface_v1_set_anchor(window->layerSurface, window->alignAnchor);
    if (window->position == MENU_WIN_POS_BOTTOM)
    {
        zwlr_layer_surface_v1_set_margin(
            window->layerSurface, 0, 0, window->windowGap * window->scale, 0
        );
    }
    else
    {
        zwlr_layer_surface_v1_set_margin(
            window->layerSurface, window->windowGap * window->scale, 0, 0, 0
        );
    }
    wl_surface_commit(window->surface);
    wl_display_roundtrip(display);
}

bool
windowRender(WaylandWindow* window, struct wl_display* display, Menu* menu)
{
    assert(window), assert(menu);

    resizeWindow(window, menu);

    Buffer* buffer = nextBuffer(window);
    if (!buffer)
    {
        errorMsg("Could not get buffer while rendering.");
        return false;
    }

    menu->width = buffer->width;
    menu->height = buffer->height;
    window->render(&buffer->cairo, menu);
    cairo_surface_flush(buffer->cairo.surface);

    moveResizeWindow(window, display);

    wl_surface_damage_buffer(window->surface, 0, 0, buffer->width, buffer->height);
    wl_surface_attach(window->surface, buffer->buffer, 0, 0);
    wl_surface_commit(window->surface);
    buffer->busy = true;
    window->renderPending = false;

    return true;
}

void
windowDestroy(WaylandWindow* window)
{
    assert(window);

    for (size_t i = 0; i < 2; i++)
    {
        destroyBuffer(&window->buffers[i]);
    }

    if (window->layerSurface) zwlr_layer_surface_v1_destroy(window->layerSurface);
    if (window->surface) wl_surface_destroy(window->surface);
}

static void
layerSurfaceConfigure(
    void* data,
    struct zwlr_layer_surface_v1* layerSurface,
    uint32_t serial,
    uint32_t width,
    uint32_t height)
{
    WaylandWindow* window = data;
    window->width = width;
    window->height = height;
    zwlr_layer_surface_v1_ack_configure(layerSurface, serial);
}

static void
layerSurfaceClosed(void* data, struct zwlr_layer_surface_v1* layerSurface)
{
    WaylandWindow* window = data;
    zwlr_layer_surface_v1_destroy(layerSurface);
    wl_surface_destroy(window->surface);
    exit(EX_SOFTWARE);
}

static const struct zwlr_layer_surface_v1_listener layerSurfaceListener = {
    .configure = layerSurfaceConfigure,
    .closed = layerSurfaceClosed,
};

static uint32_t
getWindowWidth(WaylandWindow* window)
{
    assert(window);

    return window->width;
}

static uint32_t
getWindowHeight(WaylandWindow* window, Menu* menu)
{
    assert(window), assert(menu);

    return cairoGetHeight(menu, getThrowawaySurface(window), window->maxHeight);;
}

void
windowGrabKeyboard(WaylandWindow* window, struct wl_display* display, bool grab)
{
    assert(window);

    zwlr_layer_surface_v1_set_keyboard_interactivity(window->layerSurface, grab);
    wl_surface_commit(window->surface);
    wl_display_roundtrip(display);
}

void
windowSetOverlap(WaylandWindow* window, struct wl_display* display, bool overlap)
{
    assert(window);

    zwlr_layer_surface_v1_set_exclusive_zone(window->layerSurface, overlap ? -1 : 0); /* or ... -overlap */
    wl_surface_commit(window->surface);
    wl_display_roundtrip(display);
}

bool
windowCreate(
    WaylandWindow* window,
    struct wl_display* display,
    struct wl_shm* shm,
    struct wl_output* wlOutput,
    struct zwlr_layer_shell_v1* layerShell,
    struct wl_surface* surface,
    Menu* menu)
{
    assert(window), assert(menu);

    if (!layerShell) return false;

    enum zwlr_layer_shell_v1_layer layer = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
    window->layerSurface = zwlr_layer_shell_v1_get_layer_surface(
        layerShell, surface, wlOutput, layer, "menu"
    );

    if (!window->layerSurface) return false;

    zwlr_layer_surface_v1_add_listener(window->layerSurface, &layerSurfaceListener, window);
    window->alignAnchor = getAlignAnchor(window->position);
    zwlr_layer_surface_v1_set_anchor(window->layerSurface, window->alignAnchor);
    zwlr_layer_surface_v1_set_size(window->layerSurface, 0, 32);

    wl_surface_commit(surface);
    wl_display_roundtrip(display);

    zwlr_layer_surface_v1_set_size(
        window->layerSurface, getWindowWidth(window), getWindowHeight(window, menu)
    );

    window->shm = shm;
    window->surface = surface;

    cairoInitPaint(menu, &window->paint);

    return true;
}

