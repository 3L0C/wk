#ifndef WK_LIB_TYPES_H_
#define WK_LIB_TYPES_H_

#include "common.h"

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

#endif /* WK_LIB_TYPES_H_ */
