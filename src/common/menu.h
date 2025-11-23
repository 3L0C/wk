#ifndef WK_COMMON_MENU_H_
#define WK_COMMON_MENU_H_

#include <stdint.h>
#include <time.h>

#include "common/arena.h"
#include "common/array.h"
#include "key_chord.h"

#define MENU_MIN_WIDTH 80

typedef void (*CleanupFP)(void* xp);

typedef uint8_t ForegroundColor;
enum
{
    FOREGROUND_COLOR_KEY,
    FOREGROUND_COLOR_DELIMITER,
    FOREGROUND_COLOR_PREFIX,
    FOREGROUND_COLOR_CHORD,
    FOREGROUND_COLOR_TITLE,
    FOREGROUND_COLOR_LAST,
};

typedef uint8_t MenuColor;
enum
{
    MENU_COLOR_KEY,
    MENU_COLOR_DELIMITER,
    MENU_COLOR_PREFIX,
    MENU_COLOR_CHORD,
    MENU_COLOR_TITLE,
    MENU_COLOR_BACKGROUND,
    MENU_COLOR_BORDER,
    MENU_COLOR_LAST
};

typedef uint8_t MenuStatus;
enum
{
    MENU_STATUS_RUNNING,
    MENU_STATUS_DAMAGED,
    MENU_STATUS_EXIT_OK,
    MENU_STATUS_EXIT_SOFTWARE,
};

typedef uint8_t MenuPosition;
enum
{
    MENU_POS_BOTTOM,
    MENU_POS_TOP
};

typedef struct
{
    const char* hex;
    uint8_t     r;
    uint8_t     g;
    uint8_t     b;
    uint8_t     a;
} MenuHexColor;

typedef struct
{
    const char* key;
    const char* value;
} UserVar;

typedef struct
{
    const char*  delimiter;
    const char*  shell;
    const char*  title;
    const char*  font;
    const char*  titleFont;
    const char*  implicitArrayKeys;
    double       borderRadius;
    MenuHexColor colors[MENU_COLOR_LAST];
    struct Client
    {
        const char* keys;
        const char* transpile;
        const char* wksFile;
        Array       script;
        bool        tryScript;
    } client;
    struct timespec timer;
    CleanupFP       cleanupfp;
    Array           userVars;
    Array           compiledKeyChords;
    Array*          builtinKeyChords;
    Array*          keyChords;
    Array*          keyChordsHead;
    void*           xp;
    Arena           arena;

    uint32_t maxCols;
    int32_t  menuWidth;
    int32_t  menuGap;
    uint32_t wpadding;
    uint32_t hpadding;
    int32_t  tablePadding;
    uint32_t cellHeight;
    uint32_t titleHeight;
    uint32_t rows;
    uint32_t cols;
    uint32_t width;
    uint32_t height;
    uint32_t borderWidth;
    uint32_t delay;
    uint32_t keepDelay;
    String   wrapCmd;

    MenuPosition position;
    bool         debug;
    bool         sort;
    bool         dirty;
} Menu;

int        menuDisplay(Menu* menu);
void       menuFree(Menu* menu);
MenuStatus menuHandleKeypress(Menu* menu, const Key* key);
void       menuInit(Menu* menu);
bool       menuIsDelayed(Menu* menu);
void       menuParseArgs(Menu* menu, int* argc, char*** argv);
void       menuResetTimer(Menu* menu);
void       menuSetColor(Menu* menu, const char* color, MenuColor colorType);
void       menuSetWrapCmd(Menu* menu, const char* cmd);
MenuStatus menuSpawn(const Menu* menu, const KeyChord* keyChord, const String* cmd, bool sync);
bool       menuTryStdin(Menu* menu);

bool menuStatusIsError(MenuStatus status);
bool menuStatusIsRunning(MenuStatus status);

#endif /* WK_COMMON_MENU_H_ */
