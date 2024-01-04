#include <assert.h>
#include <stdio.h>

#include "client.h"
#include "common.h"
#include "config.h"
#include "types.h"

void
initClient(Client* client, const Chord* chords)
{
    assert(client && chords);

    static const char* defaultFont = "monospace 10";
    size_t fontCount = sizeof(fonts) / sizeof(fonts[0]);

    if (fontCount > MAX_FONTS)
    {
        fprintf(
            stderr,
            "[WARNING] Cannot have more than %d fonts in 'config.h'.\n",
            MAX_FONTS
        );
    }

    client->fontSize = MAX_FONTS;
    if (fontCount == 0)
    {
        fprintf(stderr, "[WARNING] No fonts defined in 'config.h'. Setting to '%s'.\n", defaultFont);
        client->fontCount = 1;
        client->fonts[0] = defaultFont;
    }
    else
    {
        client->fontCount = fontCount < MAX_FONTS ? fontCount : MAX_FONTS;
        for (unsigned int i = 0; i < client->fontCount; i++)
        {
            client->fonts[i] = fonts[i];
        }
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
    client->chordsFile = NULL;
    client->tryScript = false;
    client->script = NULL;
    client->scriptCapacity = 0;
    client->scriptCount = 0;
    client->chords = chords;
    client->debug = false;
}
