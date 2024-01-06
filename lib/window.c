#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "common.h"
#include "client.h"
#include "types.h"
#include "window.h"
#include "x11/window.h"
#include "wayland/window.h"

static bool
initColor(WkHexColor* hexColor, const char* color)
{
    assert(hexColor && color);

    unsigned int r, g, b, a = 255;
    int count = sscanf(color, "#%2x%2x%2x%2x", &r, &b, &g, &a);

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

static void
initFonts(WkFontArray* fonts, Client* client)
{
    assert(fonts && client);

    fonts->fontCount = client->fontCount;
    fonts->fontSize = client->fontSize;
    for (size_t i = 0; i < fonts->fontCount; i++)
    {
        fonts->fonts[i] = client->fonts[i];
    }
}

void
initProperties(WkProperties* props, Client* client)
{
    assert(props && client);

    props->delimiter = client->delimiter;
    props->maxCols = client->maxCols;
    props->desiredWidth = client->windowWidth;
    props->desiredHeight = client->windowHeight;
    props->position = (client->windowPosition ? WK_WIN_POS_TOP : WK_WIN_POS_BOTTOM);
    props->borderWidth = client->borderWidth;
    initColors(props->colors, client);
    props->shell = client->shell;
    initFonts(&props->fonts, client);
    props->chords = client->chords;
    props->debug = client->debug;
}

void
pressKeys(WkProperties* props, const char* keys)
{
    assert(props);
    if (!keys) return;
}

int
run(WkProperties* props)
{
    if (getenv("WAYLAND_DISPLAY") || getenv("WAYLAND_SOCKET"))
    {
        return runWayland(props);
    }
    else
    {
        return runX11(props);
    }
    return EX_SOFTWARE;
}
