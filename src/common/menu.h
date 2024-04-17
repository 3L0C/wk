#ifndef WK_COMMON_MENU_H_
#define WK_COMMON_MENU_H_

#include <stdint.h>

#include "string.h"
#include "key_chord.h"

#define MENU_MIN_WIDTH 80

typedef void (*CleanupFP)(void* xp);

typedef enum
{
    MENU_COLOR_FOREGROUND,
    MENU_COLOR_BACKGROUND,
    MENU_COLOR_BORDER,
    MENU_COLOR_LAST
} MenuColor;

typedef struct
{
    const char* hex;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} MenuHexColor;

typedef enum
{
    MENU_STATUS_RUNNING,
    MENU_STATUS_DAMAGED,
    MENU_STATUS_EXIT_OK,
    MENU_STATUS_EXIT_SOFTWARE,
} MenuStatus;

typedef enum
{
    MENU_WIN_POS_BOTTOM,
    MENU_WIN_POS_TOP
} MenuWindowPosition;

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
    MenuWindowPosition position;
    uint32_t borderWidth;
    double borderRadius;
    MenuHexColor colors[MENU_COLOR_LAST];
    const char* shell;
    const char* font;
    KeyChord* keyChords;
    KeyChord* keyChordsHead;
    uint32_t keyChordCount;
    bool debug;
    bool sort;
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
} Menu;

void countMenuKeyChords(Menu* menu);
int displayMenu(Menu* menu);
void initMenu(Menu* menu, KeyChord* keyChords);
void setMenuColor(Menu* menu, const char* color, MenuColor colorType);

#endif /* WK_COMMON_MENU_H_ */
