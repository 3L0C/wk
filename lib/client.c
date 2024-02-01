#include <assert.h>
#include <stdio.h>

#include "client.h"
#include "common.h"
#include "config.h"
#include "types.h"

void
initClient(Client* client, Chord* chords)
{
    assert(client && chords);

    client->delimiter = delimiter;
    client->maxCols = maxCols;
    client->windowWidth = windowWidth;
    client->windowGap = windowGap;
    client->wpadding = widthPadding;
    client->hpadding = heightPadding;
    client->windowPosition = windowPosition;
    client->borderWidth = borderWidth;
    client->foreground = foreground;
    client->background = background;
    client->border = border;
    client->shell = shell;
    client->font = font;
    client->keys = NULL;
    client->transpile = NULL;
    client->chordsFile = NULL;
    client->tryScript = false;
    client->script = NULL;
    client->scriptCapacity = 0;
    client->scriptCount = 0;
    client->chords = chords;
    client->debug = false;
}
