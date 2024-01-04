#include <assert.h>
#include <stdio.h>

#include "common.h"
#include "client.h"
#include "types.h"
#include "window.h"

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
initWindow(WkWindow* window, Client* client)
{
    assert(window && client);

    window->delimiter = client->delimiter;
    window->maxCols = client->maxCols;
    window->desiredWidth = client->windowWidth;
    window->desiredHeight = client->windowHeight;
    window->position = (client->windowPosition ? WK_WIN_POS_TOP : WK_WIN_POS_BOTTOM);
    window->borderWidth = client->borderWidth;
    initColors(window->colors, client);
    window->shell = client->shell;
    initFonts(&window->fonts, client);
    window->chords = client->chords;
    window->debug = client->debug;
}

void
pressKeys(WkWindow* window, const char* keys)
{
    assert(window);
    if (!keys) return;
}
