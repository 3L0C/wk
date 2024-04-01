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

#include "lib/cairo.h"
#include "lib/common.h"
#include "lib/memory.h"
#include "lib/properties.h"

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
    CairoPaint* paint
)
{
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
    for (int32_t i = 0; i < 2; i++)
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
getAlignAnchor(WkWindowPosition position)
{
    uint32_t anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;

    if (position == WK_WIN_POS_BOTTOM)
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
    return cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, window->width * window->scale, window->height * window->scale
    );
}

static void
resizeWinWidth(WaylandWindow* window, WkProperties* props)
{
    int32_t windowWidth = props->windowWidth;
    Output* output = window->wayland->selectedOutput;
    if (windowWidth < 0)
    {
        /* set width to half the size of the screen */
        /* window->x = (output->width / 4); */
        window->width = output->width / 2;
    }
    else if (windowWidth == 0 || (uint32_t)windowWidth > output->width)
    {
        /* make the window as wide as the screen */
        /* window->x = 0; */
        window->width = output->width;
    }
    else
    {
        /* set the width to the desired user setting */
        /* window->x = (output->width - windowWidth) / 2; /\* position in the middle *\/ */
        window->width = windowWidth;
    }
}

static void
resizeWinHeight(WaylandWindow* window, WkProperties* props)
{
    int32_t windowGap = props->windowGap;
    Output* output = window->wayland->selectedOutput;
    window->maxHeight = output->height;

    if (windowGap < 0)
    {
        window->windowGap = (output->height / 10);
    }
    else if (windowGap == 0 || (uint32_t)windowGap > output->height)
    {
        window->windowGap = 0;
    }
    else
    {
        window->windowGap = windowGap;
    }

    if (window->height >= output->height)
    {
        window->windowGap = 0;
        window->height = output->height;
    }
}

static void
resizeWindow(WaylandWindow* window, WkProperties* props)
{
    resizeWinWidth(window, props);
    resizeWinHeight(window, props);
}

static void
moveResizeWindow(WaylandWindow* window, struct wl_display* display)
{
    zwlr_layer_surface_v1_set_size(
        window->layerSurface, window->width * window->scale, window->height * window->scale
    );
    zwlr_layer_surface_v1_set_anchor(window->layerSurface, window->alignAnchor);
    if (window->position == WK_WIN_POS_BOTTOM)
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

/* static void */
/* moveResizeWindow(WaylandWindow* window, struct wl_display* display) */
/* { */
/*     fprintf(stderr, "lib/wayland/window.c:moveResizeWindow:360\n"); */
/*     /\* uint32_t rootW = window->maxWidth; *\/ */
/*     uint32_t rootH = window->maxHeight; */

/*     window->y = (rootH / 10); */

/*     if (window->position == WK_WIN_POS_BOTTOM) */
/*     { */
/*         window->y = rootH - window->height - window->y; */
/*     } */

/*     zwlr_layer_surface_v1_set_size( */
/*         window->layerSurface, window->width * window->scale, window->height * window->scale */
/*     ); */
/*     zwlr_layer_surface_v1_set_anchor(window->layerSurface, window->alignAnchor); */
/*     zwlr_layer_surface_v1_set_margin(window->layerSurface, window->y * window->scale, 0, 0, 0); */
/*     wl_surface_commit(window->surface); */
/*     wl_display_roundtrip(display); */
/* } */

bool
windowRender(WaylandWindow* window, struct wl_display* display, WkProperties* props)
{
    assert(window && props);

    uint32_t oldh = window->height * window->scale;
    window->height = cairoGetHeight(props, getThrowawaySurface(window), window->maxHeight);
    resizeWindow(window, props);

    Buffer* buffer = nextBuffer(window);
    if (!buffer)
    {
        errorMsg("Could not get buffer while rendering.");
        return false;
    }

    props->width = buffer->width;
    props->height = buffer->height;
    window->render(&buffer->cairo, props);
    cairo_surface_flush(buffer->cairo.surface);

    moveResizeWindow(window, display);
    if (oldh != window->height)
    {
        moveResizeWindow(window, display);
    }

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

    for (int32_t i = 0; i < 2; i++)
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
    uint32_t height
)
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
    return window->width;
}

static uint32_t
getWindowHeight(WaylandWindow* window, WkProperties* props)
{
    return cairoGetHeight(props, getThrowawaySurface(window), window->maxHeight);;
}

/* void */
/* windowSetWidth( */
/*     WaylandWindow* window, */
/*     struct wl_display* display, */
/*     uint32_t margin, */
/*     float factor, */
/*     WkProperties* props */
/* ) */
/* { */
/*     fprintf(stderr, "lib/wayland/window.c:windowSetWidth:473\n"); */
/*     assert(window); */

/*     if (window->hpadding == margin && window->widthFactor == factor) return; */

/*     window->hpadding = margin; */
/*     window->widthFactor = factor; */

/*     zwlr_layer_surface_v1_set_anchor(window->layerSurface, window->alignAnchor); */
/*     zwlr_layer_surface_v1_set_size( */
/*         window->layerSurface, getWindowWidth(window), getWindowHeight(window, props) */
/*     ); */

/*     wl_surface_commit(window->surface); */
/*     wl_display_roundtrip(display); */
/* } */

void
windowSetAlign(WaylandWindow* window, struct wl_display* display, WkWindowPosition position)
{
    assert(window);

    if (window->position == position) return;

    window->position = position;
    window->alignAnchor = getAlignAnchor(position);

    zwlr_layer_surface_v1_set_anchor(window->layerSurface, window->alignAnchor);
    wl_surface_commit(window->surface);
    wl_display_roundtrip(display);
}

/* void */
/* windowSetYOffset(WaylandWindow* window, struct wl_display* display, int32_t yOffset) */
/* { */
/*     fprintf(stderr, "lib/wayland/window.c:windowSetYOffset:509\n"); */
/*     assert(window); */

/*     if (window->yOffset == yOffset) return; */

/*     window->yOffset = yOffset; */

/*     zwlr_layer_surface_v1_set_margin(window->layerSurface, window->yOffset, 0, 0, 0); */
/*     wl_surface_commit(window->surface); */
/*     wl_display_roundtrip(display); */
/* } */

void
windowGrabKeyboard(WaylandWindow* window, struct wl_display* display, bool grab)
{
    zwlr_layer_surface_v1_set_keyboard_interactivity(window->layerSurface, grab);
    wl_surface_commit(window->surface);
    wl_display_roundtrip(display);
}

void
windowSetOverlap(WaylandWindow* window, struct wl_display* display, bool overlap)
{
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
    WkProperties* props
)
{
    assert(window);

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
        window->layerSurface, getWindowWidth(window), getWindowHeight(window, props)
    );

    window->shm = shm;
    window->surface = surface;

    cairoInitPaint(props, &window->paint);

    return true;
}

