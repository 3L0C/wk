#ifndef WK_LIB_TYPES_H_
#define WK_LIB_TYPES_H_

#include "common.h"

#define IS_FLAG(flags, test)    (((flags) & (test)) == (test))
#define IS_MOD(mod)             (mod)
#define IS_CTRL(mod)            (IS_FLAG((mod), WK_MOD_CTRL))
#define IS_ALT(mod)             (IS_FLAG((mod), WK_MOD_ALT))
#define IS_HYPER(mod)           (IS_FLAG((mod), WK_MOD_HYPER))
#define IS_SHIFT(mod)           (IS_FLAG((mod), WK_MOD_SHIFT))
#define CHORD_FLAG(chord, flag) (IS_FLAG((chord)->flags, (flag)))

#define NULL_CHORD  \
    {                                                       \
    /*  mods,        special,         key,  desc, hint, */  \
        WK_MOD_NONE, WK_SPECIAL_NONE, NULL, NULL, NULL,     \
    /*  command, */                                         \
        NULL,                                               \
    /*  before, */                                          \
        NULL,                                               \
    /*  after, */                                           \
        NULL,                                               \
    /*  flags, */                                           \
        WK_FLAG_DEFAULTS,                                   \
        NULL                                                \
    }
#define PREFIX(...) (Chord[]){ __VA_ARGS__, NULL_CHORD }
#define CHORDS(...) { __VA_ARGS__, NULL_CHORD }

typedef enum
{
    WK_MOD_NONE     = (0),
    WK_MOD_CTRL     = (1<<0),
    WK_MOD_ALT      = (1<<1),
    WK_MOD_HYPER    = (1<<2),
    WK_MOD_SHIFT    = (1<<3),
} WkMod;

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

typedef enum
{
    WK_FLAG_DEFAULTS        = (0),
    WK_FLAG_KEEP            = (1<<0),
    WK_FLAG_UNHOOK          = (1<<1),
    WK_FLAG_NOBEFORE        = (1<<2),
    WK_FLAG_NOAFTER         = (1<<3),
    WK_FLAG_WRITE           = (1<<4),
    WK_FLAG_SYNC_COMMAND    = (1<<5),
    WK_FLAG_BEFORE_ASYNC    = (1<<6),
    WK_FLAG_AFTER_SYNC      = (1<<7),
} WkFlags;

typedef struct Chord
{
    const WkMod mods;
    const SpecialType special;
    const char* key;
    const char* description;
    const char* hint;
    const char* command;
    const char* before;
    const char* after;
    const WkFlags flags;
    struct Chord* chords;
} Chord;

typedef struct
{
    WkMod mods;
    SpecialType special;
    const char* key;
    int len;
} Key;

#endif /* WK_LIB_TYPES_H_ */
