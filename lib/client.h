#ifndef WK_CLIENT_H_
#define WK_CLIENT_H_

#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "types.h"

typedef struct
{
    const char* delimiter;
    size_t maxCols;
    int windowWidth;
    int windowGap;
    uint32_t wpadding;
    uint32_t hpadding;
    size_t windowPosition;
    size_t borderWidth;
    const char* foreground;
    const char* background;
    const char* border;
    const char* shell;
    const char* font;
    const char* keys;
    const char* parse;
    const char* chordsFile;
    bool tryScript;
    char* script;
    size_t scriptCapacity;
    size_t scriptCount;
    const Chord* chords;
    bool debug;
} Client;

void initClient(Client* client, const Chord* chords);

#endif /* WK_CLIENT_H_ */
