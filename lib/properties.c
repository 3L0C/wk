#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "client.h"
#include "properties.h"

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
initProperties(WkProperties* props, Client* client)
{
    assert(props && client);

    props->delimiter = client->delimiter;
    props->maxCols = client->maxCols;
    props->windowWidth = client->windowWidth;
    props->windowGap = client->windowGap;
    props->wpadding = client->wpadding;
    props->hpadding = client->hpadding;
    props->cellHeight = 0;
    props->rows = 0;
    props->cols = 0;
    props->width = 0;
    props->height = 0;
    props->position = (client->windowPosition ? WK_WIN_POS_TOP : WK_WIN_POS_BOTTOM);
    props->borderWidth = client->borderWidth;
    props->borderRadius = client->borderRadius;
    initColors(props->colors, client);
    props->shell = client->shell;
    props->font = client->font;
    props->chords = client->chords;
    props->chordCount = 0;
    props->debug = client->debug;
    props->dirty = true;
    props->cleanupfp = NULL;
    props->xp = NULL;
}

