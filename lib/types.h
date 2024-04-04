#ifndef WK_LIB_TYPES_H_
#define WK_LIB_TYPES_H_

#include "common.h"

/* mod macros */
#define IS_MOD_FLAG(mods)    (((mods) & MOD_CTRL) || \
                              ((mods) & MOD_ALT)  || \
                              ((mods) & MOD_LOGO) || \
                              ((mods) & MOD_SHIFT))
#define MAKE_MODS(                      \
    _ctrl, _alt, _hyper, _shift         \
)                                       \
    (WkMods){                           \
        .ctrl = (_ctrl),                \
        .alt = (_alt),                  \
        .hyper = (_hyper),              \
        .shift = (_shift),              \
    }
#define RESET_MODS(mods)                \
    ((mods).ctrl = false,               \
     (mods).alt = false,                \
     (mods).hyper = false,              \
     (mods).shift = false)
#define IS_MOD(mods)                    \
    ((mods).ctrl    ||                  \
     (mods).alt     ||                  \
     (mods).hyper   ||                  \
     (mods).shift)
#define COUNT_MODS(mods)                \
    ((mods).ctrl    +                   \
     (mods).alt     +                   \
     (mods).hyper   +                   \
     (mods).shift)

/* flag macros */
#define MAKE_FLAGS(                     \
    _keep, _close, _inherit, _unhook,   \
    _nobefore, _noafter, _write,        \
    _syncCommand, _beforeAsync,         \
    _afterSync                          \
)                                       \
    (WkFlags){                          \
        .keep = (_keep),                \
        .close = (_close),              \
        .inherit = (_inherit),          \
        .unhook = (_unhook),            \
        .nobefore = (_nobefore),        \
        .noafter = (_noafter),          \
        .write = (_write),              \
        .syncCommand = (_syncCommand),  \
        .beforeAsync = (_beforeAsync),  \
        .afterSync = (_afterSync),      \
    }
#define RESET_FLAGS(flags)              \
    ((flags).keep = false,              \
     (flags).close = false,             \
     (flags).inherit = false,           \
     (flags).unhook = false,            \
     (flags).nobefore = false,          \
     (flags).noafter = false,           \
     (flags).write = false,             \
     (flags).syncCommand = false,       \
     (flags).beforeAsync = false,       \
     (flags).afterSync = false)
#define HAS_FLAG(flags)                 \
    ((flags).keep        ||             \
     (flags).close       ||             \
     (flags).inherit     ||             \
     (flags).unhook      ||             \
     (flags).nobefore    ||             \
     (flags).noafter     ||             \
     (flags).write       ||             \
     (flags).syncCommand ||             \
     (flags).beforeAsync ||             \
     (flags).afterSync)
#define COUNT_FLAGS(flags)              \
    (((flags).keep)        +            \
     ((flags).close)       +            \
     ((flags).inherit)     +            \
     ((flags).unhook)      +            \
     ((flags).nobefore)    +            \
     ((flags).noafter)     +            \
     ((flags).write)       +            \
     ((flags).syncCommand) +            \
     ((flags).beforeAsync) +            \
     ((flags).afterSync))

/* chord macros */
#define NULL_CHORD  \
    {                                   \
    /*  mods,        special, */        \
        (WkMods){0}, WK_SPECIAL_NONE,   \
    /*  key,  desc, hint, */            \
        NULL, NULL, NULL,               \
    /*  command, */                     \
        NULL,                           \
    /*  before, */                      \
        NULL,                           \
    /*  after, */                       \
        NULL,                           \
    /*  flags,        chords */         \
        (WkFlags){0}, NULL              \
    }
#define PREFIX(...) (Chord[]){ __VA_ARGS__, NULL_CHORD }
#define CHORDS(...) { __VA_ARGS__, NULL_CHORD }

typedef enum
{
    WK_KEY_IS_STRICTLY_MOD,
    WK_KEY_IS_SPECIAL,
    WK_KEY_IS_NORMAL,
    WK_KEY_IS_UNKNOWN,
} WkKeyType;

typedef struct
{
    bool ctrl:1;
    bool alt:1;
    bool hyper:1;
    bool shift:1;
} WkMods;

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
} WkSpecial;

typedef enum
{
    WK_STATUS_RUNNING,
    WK_STATUS_DAMAGED,
    WK_STATUS_EXIT_OK,
    WK_STATUS_EXIT_SOFTWARE,
} WkStatus;

typedef struct
{
    bool keep:1;
    bool close:1;
    bool inherit:1;
    bool unhook:1;
    bool nobefore:1;
    bool noafter:1;
    bool write:1;
    bool syncCommand:1;
    bool beforeAsync:1;
    bool afterSync:1;
} WkFlags;

typedef struct Chord
{
    WkMods mods;
    WkSpecial special;
    char* key;
    char* description;
    char* hint;
    char* command;
    char* before;
    char* after;
    WkFlags flags;
    struct Chord* chords;
} Chord;

typedef struct
{
    WkMods mods;
    WkSpecial special;
    const char* key;
    int len;
} Key;

bool keyIsNormal(Key* key);
bool keyIsSpecial(Key* key);
bool keyIsStrictlyMod(Key* key);

#endif /* WK_LIB_TYPES_H_ */
