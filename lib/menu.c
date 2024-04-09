#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#ifdef WK_X11_BACKEND
#include "x11/window.h"
#endif

#ifdef WK_WAYLAND_BACKEND
#include "wayland/wayland.h"
#endif

#include "client.h"
#include "debug.h"
#include "menu.h"

static bool
initColor(WkHexColor* hexColor, const char* color)
{
    assert(hexColor && color);

    unsigned int r, g, b, a = 255;
    int count = sscanf(color, "#%2x%2x%2x%2x", &r, &g, &b, &a);

    if (count != 3 && count != 4) return false;

    hexColor->hex = color;
    hexColor->r = r;
    hexColor->g = g;
    hexColor->b = b;
    hexColor->a = a;

    return true;
}

static void
initColors(WkHexColor* hexColors, Client* client)
{
    assert(hexColors && client);

    static const char* defaultColors[WK_COLOR_LAST] = {
        "#DCD7BA", "#181616", "#7FB4CA"
    };

    const char* colors[WK_COLOR_LAST] = {
        client->foreground, client->background, client->border
    };
    for (int i = 0; i < WK_COLOR_LAST; i++)
    {
        if (!initColor(&hexColors[i], colors[i]))
        {
            char* colorType;
            fprintf(stderr, "[WARNING] Invalid color string '%s' ", colors[i]);
            switch (i)
            {
            case WK_COLOR_FOREGROUND: colorType = "foreground"; break;
            case WK_COLOR_BACKGROUND: colorType = "background"; break;
            case WK_COLOR_BORDER: colorType = "border"; break;
            default: colorType = "UNKNOWN"; break;
            }
            fprintf(stderr, "setting %s to '%s'.\n", colorType, defaultColors[i]);
            initColor(&hexColors[i], defaultColors[i]);
        }
    }
}

void
initMenu(WkMenu* menu, Client* client)
{
    assert(menu && client);

    menu->delimiter = client->delimiter;
    menu->maxCols = client->maxCols;
    menu->windowWidth = client->windowWidth;
    menu->windowGap = client->windowGap;
    menu->wpadding = client->wpadding;
    menu->hpadding = client->hpadding;
    menu->cellHeight = 0;
    menu->rows = 0;
    menu->cols = 0;
    menu->width = 0;
    menu->height = 0;
    menu->position = (client->windowPosition ? WK_WIN_POS_TOP : WK_WIN_POS_BOTTOM);
    menu->borderWidth = client->borderWidth;
    menu->borderRadius = client->borderRadius;
    initColors(menu->colors, client);
    menu->shell = client->shell;
    menu->font = client->font;
    menu->chords = client->chords;
    menu->chordCount = 0;
    menu->debug = client->debug;
    menu->dirty = true;
    menu->cleanupfp = NULL;
    menu->xp = NULL;
}

int
displayMenu(WkMenu* props)
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
    errorMsg("Can only run under X11 and/or Wayland.");
    return EX_SOFTWARE;
}
