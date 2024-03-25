#include <stdlib.h>
#include <sysexits.h>

#include "common.h"
#include "debug.h"
#include "properties.h"
#include "window.h"

#ifdef WK_X11_BACKEND
#include "x11/window.h"
#endif

#ifdef WK_WAYLAND_BACKEND
#include "wayland/wayland.h"
#endif

int
run(WkProperties* props)
{
#ifdef WK_WAYLAND_BACKEND
    if (getenv("WAYLAND_DISPLAY") || getenv("WAYLAND_SOCKET"))
    {
        debugMsg(props->debug, "Running on wayland.");
        return runWayland(props);
    }
#endif
#ifdef WK_X11_BACKEND
    debugMsg(props->debug, "Running on x11.");
    return runX11(props);
#endif
    errorMsg("Can only run under X11 and Wayland.");
    return EX_SOFTWARE;
}
