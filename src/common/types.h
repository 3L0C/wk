#ifndef WK_LIB_TYPES_H_
#define WK_LIB_TYPES_H_

#include <stdbool.h>

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
    _keep, _close, _inherit, _ignore,   \
    _unhook, _deflag,                   \
    _nobefore, _noafter, _write,        \
    _syncCommand, _beforeSync,          \
    _afterSync                          \
)                                       \
    (WkFlags){                          \
        .keep = (_keep),                \
        .close = (_close),              \
        .inherit = (_inherit),          \
        .ignore = (_ignore),            \
        .unhook = (_unhook),            \
        .deflag = (_deflag),            \
        .nobefore = (_nobefore),        \
        .noafter = (_noafter),          \
        .write = (_write),              \
        .syncCommand = (_syncCommand),  \
        .beforeSync = (_beforeSync),    \
        .afterSync = (_afterSync),      \
    }

#define RESET_FLAGS(flags)              \
    ((flags).keep = false,              \
     (flags).close = false,             \
     (flags).inherit = false,           \
     (flags).ignore = false,            \
     (flags).unhook = false,            \
     (flags).deflag = false,            \
     (flags).nobefore = false,          \
     (flags).noafter = false,           \
     (flags).write = false,             \
     (flags).syncCommand = false,       \
     (flags).beforeSync = false,        \
     (flags).afterSync = false)

#define HAS_FLAG(flags)                 \
    ((flags).keep        ||             \
     (flags).close       ||             \
     (flags).inherit     ||             \
     (flags).ignore      ||             \
     (flags).unhook      ||             \
     (flags).deflag      ||             \
     (flags).nobefore    ||             \
     (flags).noafter     ||             \
     (flags).write       ||             \
     (flags).syncCommand ||             \
     (flags).beforeSync  ||             \
     (flags).afterSync)

#define COUNT_FLAGS(flags)              \
    (((flags).keep)        +            \
     ((flags).close)       +            \
     ((flags).inherit)     +            \
     ((flags).ignore)      +            \
     ((flags).unhook)      +            \
     ((flags).deflag)      +            \
     ((flags).nobefore)    +            \
     ((flags).noafter)     +            \
     ((flags).write)       +            \
     ((flags).syncCommand) +            \
     ((flags).beforeSync)  +            \
     ((flags).afterSync))

#define COPY_FLAGS(from, to)                    \
    do                                          \
    {                                           \
        (to).keep = (from).keep;                \
        (to).close = (from).close;              \
        (to).inherit = (from).inherit;          \
        (to).ignore = (from).ignore;            \
        (to).unhook = (from).unhook;            \
        (to).deflag = (from).deflag;            \
        (to).nobefore = (from).nobefore;        \
        (to).noafter = (from).noafter;          \
        (to).write = (from).write;              \
        (to).syncCommand = (from).syncCommand;  \
        (to).beforeSync = (from).beforeSync;    \
        (to).afterSync = (from).afterSync;      \
    } while (false)

/* chord macros */
#define NULL_KEY_CHORD                  \
    {                                   \
        WK_KEY_CHORD_STATE_IS_NULL,     \
        (WkMods){0}, WK_SPECIAL_NONE,   \
        NULL, NULL, NULL,               \
        NULL,                           \
        NULL,                           \
        NULL,                           \
        (WkFlags){0}, NULL              \
    }

#define PREFIX(...) (WkKeyChord[]){ __VA_ARGS__, NULL_KEY_CHORD }
#define KEY_CHORDS(...) { __VA_ARGS__, NULL_KEY_CHORD }

typedef enum
{
    WK_KEY_CHORD_STATE_NOT_NULL,
    WK_KEY_CHORD_STATE_IS_NULL,
} WkKeyChordState;

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

typedef struct
{
    bool keep:1;
    bool close:1;
    bool inherit:1;
    bool ignore:1;
    bool unhook:1;
    bool deflag:1;
    bool nobefore:1;
    bool noafter:1;
    bool write:1;
    bool syncCommand:1;
    bool beforeSync:1;
    bool afterSync:1;
} WkFlags;

typedef struct KeyChord
{
    WkKeyChordState state;
    WkMods mods;
    WkSpecial special;
    char* key;
    char* description;
    char* hint;
    char* command;
    char* before;
    char* after;
    WkFlags flags;
    struct KeyChord* keyChords;
} WkKeyChord;

typedef struct
{
    WkMods mods;
    WkSpecial special;
    const char* key;
    int len;
} WkKey;

bool keyIsNormal(WkKey* key);
bool keyIsSpecial(WkKey* key);
bool keyIsStrictlyMod(WkKey* key);

#endif /* WK_LIB_TYPES_H_ */
