#ifndef WK_COMMON_MENU_H_
#define WK_COMMON_MENU_H_

#include <stdint.h>

#include "string.h"
#include "types.h"

#define MENU_MIN_WIDTH 80

typedef void (*CleanupFP)(void* xp);

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

typedef enum
{
    WK_STATUS_RUNNING,
    WK_STATUS_DAMAGED,
    WK_STATUS_EXIT_OK,
    WK_STATUS_EXIT_SOFTWARE,
} WkStatus;

typedef enum
{
    WK_WIN_POS_BOTTOM,
    WK_WIN_POS_TOP
} WkWindowPosition;

typedef struct
{
    const char* delimiter;
    uint32_t maxCols;
    int32_t  windowWidth;
    int32_t  windowGap;
    uint32_t wpadding;
    uint32_t hpadding;
    uint32_t cellHeight;
    uint32_t rows;
    uint32_t cols;
    uint32_t width;
    uint32_t height;
    WkWindowPosition position;
    uint32_t borderWidth;
    double borderRadius;
    WkHexColor colors[WK_COLOR_LAST];
    const char* shell;
    const char* font;
    WkKeyChord* keyChords;
    WkKeyChord* keyChordsHead;
    uint32_t keyChordCount;
    bool debug;
    bool dirty;
    struct Client
    {
        const char* keys;
        const char* transpile;
        const char* wksFile;
        bool tryScript;
        String script;
    } client;
    struct Garbage
    {
        char* shell;
        char* font;
        char* foregroundColor;
        char* backgroundColor;
        char* borderColor;
    } garbage;
    CleanupFP cleanupfp;
    void* xp;
} WkMenu;

void countMenuKeyChords(WkMenu* menu);
int displayMenu(WkMenu* menu);
void initMenu(WkMenu* menu, WkKeyChord* keyChords);
void setMenuColor(WkMenu* menu, const char* color, WkColor colorType);

#endif /* WK_COMMON_MENU_H_ */
