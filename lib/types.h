#ifndef WK_LIB_TYPES_H_
#define WK_LIB_TYPES_H_

#include "common.h"

#define WK_MOD_NONE     (1<<0)
#define WK_MOD_CTRL     (1<<1)
#define WK_MOD_ALT      (1<<2)
#define WK_MOD_HYPER    (1<<3)
#define WK_MOD_SHIFT    (1<<4)
#define WK_MOD_ALL      (WK_MOD_CTRL|WK_MOD_ALT|WK_MOD_HYPER|WK_MOD_SHIFT)
#define IS_MOD(mod)     ((mod) != WK_MOD_NONE && \
                         ((mod) & WK_MOD_ALL) != WK_MOD_NONE)
#define IS_CTRL(mod)    (((mod) & WK_MOD_CTRL) == WK_MOD_CTRL)
#define IS_ALT(mod)     (((mod) & WK_MOD_ALT) == WK_MOD_ALT)
#define IS_HYPER(mod)   (((mod) & WK_MOD_HYPER) == WK_MOD_HYPER)
#define IS_SHIFT(mod)   (((mod) & WK_MOD_SHIFT) == WK_MOD_SHIFT)

typedef enum
{
    WK_SPECIAL_NONE,
    WK_SPECIAL_LEFT,
    WK_SPECIAL_RIGHT,
    WK_SPECIAL_UP,
    WK_SPECIAL_DOWN,
    WK_SPECIAL_TAB,
    WK_SPECIAL_SPACE,
    WK_SPECIAL_RETURN,
    WK_SPECIAL_DELETE,
    WK_SPECIAL_ESCAPE,
    WK_SPECIAL_HOME,
    WK_SPECIAL_PAGE_UP,
    WK_SPECIAL_PAGE_DOWN,
    WK_SPECIAL_END,
    WK_SPECIAL_BEGIN,
} SpecialType;

typedef struct Chord
{
    const unsigned int mods;
    const SpecialType special;
    const char* key;
    const char* description;
    const char* hint;
    const char* command;
    const char* before;
    const char* after;
    const bool keep;
    const bool unhook;
    const bool nobefore;
    const bool noafter;
    const bool write;
    struct Chord* chords;
} Chord;

typedef struct
{
    unsigned int mods;
    SpecialType special;
    const char* key;
    int len;
} Key;

#endif /* WK_LIB_TYPES_H_ */
