#ifndef WK_CLIENT_H_
#define WK_CLIENT_H_

#include <stdbool.h>

#include "lib/common.h"

typedef struct
{
    const char* delimiter;
    unsigned int maxCols;
    int windowWidth;
    int windowHeight;
    unsigned int windowPosition;
    unsigned int borderWidth;
    const char* foreground;
    const char* background;
    const char* border;
    const char* shell;
    const char* fonts[MAX_FONTS];
    unsigned int fontCount;
    unsigned int fontSize;
    const char* keys;
    const char* parse;
    const char* chords;
    const char* script;
    bool debug;
} Client;

void initClient(Client*);

#endif /* WK_CLIENT_H_ */
