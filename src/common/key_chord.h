#ifndef WK_COMMON_KEY_CHORD_H_
#define WK_COMMON_KEY_CHORD_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* mod macros */
#define MAKE_MODS(                      \
    _ctrl, _alt, _hyper, _shift         \
)                                       \
    (Modifiers){                        \
        .ctrl = (_ctrl),                \
        .alt = (_alt),                  \
        .hyper = (_hyper),              \
        .shift = (_shift),              \
    }

/* flag macros */
#define MAKE_FLAGS(                     \
    _keep, _close, _inherit, _ignore,   \
    _unhook, _deflag,                   \
    _nobefore, _noafter,                \
    _write, _execute,                   \
    _syncCommand, _syncBefore,          \
    _syncAfter                          \
)                                       \
    (ChordFlags){                       \
        .keep = (_keep),                \
        .close = (_close),              \
        .inherit = (_inherit),          \
        .ignore = (_ignore),            \
        .unhook = (_unhook),            \
        .deflag = (_deflag),            \
        .nobefore = (_nobefore),        \
        .noafter = (_noafter),          \
        .write = (_write),              \
        .execute = (_execute),          \
        .syncCommand = (_syncCommand),  \
        .syncBefore = (_syncBefore),    \
        .syncAfter = (_syncAfter),      \
    }

/* key macros */
#define MAKE_KEY(                       \
    _ctrl, _alt, _hyper, _shift,        \
    _special,                           \
    _key, _len                          \
)                                       \
    (Key){                              \
        MAKE_MODS((_ctrl), (_alt),      \
                  (_hyper), (_shift)),  \
        (_special), (_key), (_len)      \
    }

#define MAKE_NULL_KEY \
    MAKE_KEY(false, false, false, false, \
             SPECIAL_KEY_NONE, NULL, 0)

/* chord macros */
#define NULL_KEY_CHORD                          \
    {                                           \
        KEY_CHORD_STATE_IS_NULL,                \
        MAKE_NULL_KEY,                          \
        NULL, NULL,                             \
        NULL,                                   \
        NULL,                                   \
        NULL,                                   \
        (ChordFlags){0}, NULL,                  \
    }

#define PREFIX(...) (KeyChord[]){ __VA_ARGS__, NULL_KEY_CHORD }
#define KEY_CHORDS(...) { __VA_ARGS__, NULL_KEY_CHORD }

typedef enum
{
    KEY_CHORD_STATE_NOT_NULL,
    KEY_CHORD_STATE_IS_NULL,
} KeyChordState;

typedef enum
{
    KEY_TYPE_IS_STRICTLY_MOD,
    KEY_TYPE_IS_SPECIAL,
    KEY_TYPE_IS_NORMAL,
    KEY_TYPE_IS_UNKNOWN,
} KeyType;

typedef struct
{
    bool ctrl:1;
    bool alt:1;
    bool hyper:1;
    bool shift:1;
} Modifiers;

typedef enum
{
    SPECIAL_KEY_NONE,
    SPECIAL_KEY_LEFT,
    SPECIAL_KEY_RIGHT,
    SPECIAL_KEY_UP,
    SPECIAL_KEY_DOWN,
    SPECIAL_KEY_TAB,
    SPECIAL_KEY_SPACE,
    SPECIAL_KEY_RETURN,
    SPECIAL_KEY_DELETE,
    SPECIAL_KEY_ESCAPE,
    SPECIAL_KEY_HOME,
    SPECIAL_KEY_PAGE_UP,
    SPECIAL_KEY_PAGE_DOWN,
    SPECIAL_KEY_END,
    SPECIAL_KEY_BEGIN,
} SpecialKey;

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
    bool execute:1;
    bool syncCommand:1;
    bool syncBefore:1;
    bool syncAfter:1;
} ChordFlags;

typedef struct
{
    Modifiers mods;
    SpecialKey special;
    char* repr;
    size_t len;
} Key;

typedef struct KeyChord
{
    KeyChordState state;
    Key key;
    char* description;
    char* hint;
    char* command;
    char* before;
    char* after;
    ChordFlags flags;
    struct KeyChord* keyChords;
} KeyChord;

typedef struct
{
    KeyChord** keyChords;
    size_t count;
    size_t capacity;
} ChordArray;

void copyChordFlags(ChordFlags* from, ChordFlags* to);
uint32_t countModifiers(const Modifiers* mods);
uint32_t countChordFlags(const ChordFlags* flags);
const char* getSpecialKeyRepr(const SpecialKey special);
bool hasChordFlags(const ChordFlags* flags);
void initKeyChord(KeyChord* keyChord);
void initChordArray(ChordArray* dest, KeyChord** source);
bool hasActiveModifier(const Modifiers* mods);
bool hasDefaultChordFlags(const ChordFlags* flags);
bool keysAreEqual(const Key* a, const Key* b);
bool keyIsNormal(const Key* key);
bool keyIsSpecial(const Key* key);
bool keyIsStrictlyMod(const Key* key);
void makePsuedoChordArray(ChordArray* array, KeyChord** keyChords);
KeyChord* writeChordArray(ChordArray* array, KeyChord* keyChord);

#endif /* WK_COMMON_KEY_CHORD_H_ */
