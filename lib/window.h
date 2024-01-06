#ifndef WK_LIB_WINDOW_H_
#define WK_LIB_WINDOW_H_

#include <stddef.h>

#include "common.h"
#include "client.h"
#include "types.h"

#define WINDOW_MIN_WIDTH 80

typedef enum
{
    WK_WIN_POS_BOTTOM,
    WK_WIN_POS_TOP
} WkWindowPosition;

typedef enum
{
    WK_COLOR_FOREGROUND,
    WK_COLOR_BACKGROUND,
    WK_COLOR_BORDER,
    WK_COLOR_LAST
} WkColor;

typedef struct
{
    const char* hex;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} WkHexColor;

typedef struct
{
    const char* fonts[MAX_FONTS];
    size_t fontCount;
    size_t fontSize;
} WkFontArray;

typedef struct
{
    const char* delimiter;
    size_t maxCols;
    int desiredWidth;
    int desiredHeight;
    WkWindowPosition position;
    size_t borderWidth;
    WkHexColor colors[WK_COLOR_LAST];
    const char* shell;
    WkFontArray fonts;
    const Chord* chords;
    bool debug;
} WkProperties;

void initProperties(WkProperties* props, Client* client);
void pressKeys(WkProperties* props, const char* keys);
int run(WkProperties* props);

#endif /* WK_LIB_WINDOW_H_ */
