#include <assert.h>
#include <stdio.h>

#include "lib/common.h"

#include "client.h"
#include "config.h"
#include "chords.h"

void
initClient(Client* client)
{
    assert(client);

    size_t fontCount = sizeof(fonts) / sizeof(fonts[0]);

    if (fontCount > MAX_FONTS)
    {
        fprintf(
            stderr,
            "[WARNING] Cannot have more than %d fonts in 'config.h'.\n",
            MAX_FONTS
        );
    }

    client->delimiter = delimiter;
    client->maxCols = maxCols;
    client->windowWidth = windowWidth;
    client->windowHeight = windowHeight;
    client->windowPosition = windowPosition;
    client->borderWidth = borderWidth;
    client->foreground = foreground;
    client->background = background;
    client->border = border;
    client->shell = shell;
    client->keys = NULL;
    client->parse = NULL;
    client->chords = NULL;
    client->script = NULL;
    client->debug = false;

    client->fontSize = MAX_FONTS;
    client->fontCount = fontCount < MAX_FONTS ? fontCount : MAX_FONTS;
    for (unsigned int i = 0; i < client->fontCount; i++)
    {
        client->fonts[i] = fonts[i];
    }
}
