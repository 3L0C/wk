#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#ifdef WK_X11_BACKEND
#include "runtime/x11/window.h"
#endif

#ifdef WK_WAYLAND_BACKEND
#include "runtime/wayland/wayland.h"
#endif

/* config include */
#include "config/config.h"

/* local includes */
#include "common.h"
#include "debug.h"
#include "menu.h"
#include "util.h"
#include "key_chord.h"

void
countMenuKeyChords(Menu* menu)
{
    assert(menu);

    menu->keyChordCount = countKeyChords(menu->keyChords);
}

int
displayMenu(Menu* props)
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

static bool
initColor(MenuHexColor* hexColor, const char* color)
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
initColors(MenuHexColor* hexColors)
{
    assert(hexColors);

    static const char* defaultColors[MENU_COLOR_LAST] = {
        "#DCD7BA", "#181616", "#7FB4CA"
    };

    const char* colors[MENU_COLOR_LAST] = {
        foreground, background, border
    };
    for (int i = 0; i < MENU_COLOR_LAST; i++)
    {
        if (!initColor(&hexColors[i], colors[i]))
        {
            char* colorType;
            warnMsg("Invalid color string '%s'.", colors[i]);
            switch (i)
            {
            case MENU_COLOR_FOREGROUND: colorType = "foreground"; break;
            case MENU_COLOR_BACKGROUND: colorType = "background"; break;
            case MENU_COLOR_BORDER: colorType = "border"; break;
            default: colorType = "UNKNOWN"; break;
            }
            fprintf(stderr, "setting %s to '%s'.\n", colorType, defaultColors[i]);
            initColor(&hexColors[i], defaultColors[i]);
        }
    }
}

void
initMenu(Menu* menu, KeyChord* keyChords)
{
    assert(menu);

    menu->delimiter = delimiter;
    menu->maxCols = maxCols;
    menu->windowWidth = windowWidth;
    menu->windowGap = windowGap;
    menu->wpadding = widthPadding;
    menu->hpadding = heightPadding;
    menu->cellHeight = 0;
    menu->rows = 0;
    menu->cols = 0;
    menu->width = 0;
    menu->height = 0;
    menu->position = (windowPosition ? MENU_WIN_POS_TOP : MENU_WIN_POS_BOTTOM);
    menu->borderWidth = borderWidth;
    menu->borderRadius = borderRadius;
    initColors(menu->colors);
    menu->shell = shell;
    menu->font = font;
    menu->keyChords = keyChords;
    menu->keyChords = NULL;
    menu->keyChordCount = 0;
    menu->debug = false;
    menu->dirty = true;
    menu->client.keys = NULL;
    menu->client.transpile = NULL;
    menu->client.wksFile = NULL;
    menu->client.tryScript = false;
    initString(&menu->client.script);
    menu->garbage.shell = NULL;
    menu->garbage.font = NULL;
    menu->garbage.foregroundColor = NULL;
    menu->garbage.backgroundColor = NULL;
    menu->garbage.borderColor = NULL;
    menu->cleanupfp = NULL;
    menu->xp = NULL;
}

void
setMenuColor(Menu* menu, const char* color, MenuColor colorType)
{
    assert(menu && colorType < MENU_COLOR_LAST && !(colorType < 0));

    if (!initColor(&menu->colors[colorType], color)) warnMsg("Invalid color string: '%s'.", color);
}
