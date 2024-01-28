#ifndef WK_LIB_PROPERTIES_H_
#define WK_LIB_PROPERTIES_H_

#include <stddef.h>
#include <stdint.h>

#include "common.h"
#include "client.h"
#include "types.h"

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
    const char* delimiter;
    size_t maxCols;
    int windowWidth;
    int windowGap;
    uint32_t wpadding;
    uint32_t hpadding;
    uint32_t cell_height;
    uint32_t rows;
    uint32_t cols;
    uint32_t width;
    uint32_t height;
    WkWindowPosition position;
    size_t borderWidth;
    WkHexColor colors[WK_COLOR_LAST];
    const char* shell;
    const char* font;
    const Chord* chords;
    uint32_t chordCount;
    bool debug;
    CleanupFP cleanupfp;
    void* xp;
} WkProperties;

void initProperties(WkProperties* props, Client* client);
void pressKeys(WkProperties* props, const char* keys);

#endif /* WK_LIB_PROPERTIES_H_ */
