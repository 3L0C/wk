#ifndef WK_COMMON_MENU_H_
#define WK_COMMON_MENU_H_

#include <stdint.h>
#include <time.h>

#include "string.h"
#include "key_chord.h"

#define MENU_MIN_WIDTH 80

typedef void (*CleanupFP)(void* xp);

typedef enum
{
    FOREGROUND_COLOR_KEY,
    FOREGROUND_COLOR_DELIMITER,
    FOREGROUND_COLOR_PREFIX,
    FOREGROUND_COLOR_CHORD,
    FOREGROUND_COLOR_LAST,
} ForegroundColor;

typedef enum
{
    MENU_COLOR_KEY,
    MENU_COLOR_DELIMITER,
    MENU_COLOR_PREFIX,
    MENU_COLOR_CHORD,
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
    MENU_POS_BOTTOM,
    MENU_POS_TOP
} MenuPosition;

typedef struct
{
    const char* delimiter;
    uint32_t maxCols;
    int32_t  menuWidth;
    int32_t  menuGap;
    uint32_t wpadding;
    uint32_t hpadding;
    uint32_t cellHeight;
    uint32_t rows;
    uint32_t cols;
    uint32_t width;
    uint32_t height;
    /* uint32_t maxWinWidth; */
    /* uint32_t maxWinHeight; */
    MenuPosition position;
    uint32_t borderWidth;
    double borderRadius;
    MenuHexColor colors[MENU_COLOR_LAST];
    const char* shell;
    const char* font;
    const char* implicitArrayKeys;
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
        char* implicitArrayKeys;
        char* foregroundKeyColor;
        char* foregroundDelimiterColor;
        char* foregroundPrefixColor;
        char* foregroundChordColor;
        char* backgroundColor;
        char* borderColor;
    } garbage;
    uint32_t delay;
    struct timespec timer;
    CleanupFP cleanupfp;
    void* xp;
} Menu;

void countMenuKeyChords(Menu* menu);
int displayMenu(Menu* menu);
void freeMenuGarbage(Menu* menu);
MenuStatus handleKeypress(Menu* menu, const Key* key, bool shiftIsSignificant);
void initMenu(Menu* menu, KeyChord* keyChords);
bool menuIsDelayed(Menu* menu);
void menuResetTimer(Menu* menu);
void parseArgs(Menu* menu, int* argc, char*** argv);
void setMenuColor(Menu* menu, const char* color, MenuColor colorType);
MenuStatus spawn(const Menu* menu, const char* cmd, bool async);
bool statusIsError(MenuStatus status);
bool statusIsRunning(MenuStatus status);
bool tryStdin(Menu* menu);

#endif /* WK_COMMON_MENU_H_ */
