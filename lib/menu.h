#ifndef WK_LIB_MENU_H_
#define WK_LIB_MENU_H_

#include <stddef.h>
#include <stdint.h>

#include "common.h"
#include "client.h"
#include "types.h"

#define MENU_MIN_WIDTH 80

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
    uint32_t cellHeight;
    uint32_t rows;
    uint32_t cols;
    uint32_t width;
    uint32_t height;
    WkWindowPosition position;
    size_t borderWidth;
    double borderRadius;
    WkHexColor colors[WK_COLOR_LAST];
    const char* shell;
    const char* font;
    Chord* chords;
    uint32_t chordCount;
    bool debug;
    bool dirty;
    CleanupFP cleanupfp;
    void* xp;
} WkMenu;

void initMenu(WkMenu* props, Client* client);
int displayMenu(WkMenu* menu);

#endif /* WK_LIB_MENU_H_ */
