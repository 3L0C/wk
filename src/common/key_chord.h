#ifndef WK_COMMON_KEY_CHORD_H_
#define WK_COMMON_KEY_CHORD_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    KEY_CHORD_STATE_IS_NULL,
    KEY_CHORD_STATE_NOT_NULL,
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
    SPECIAL_KEY_F1,
    SPECIAL_KEY_F2,
    SPECIAL_KEY_F3,
    SPECIAL_KEY_F4,
    SPECIAL_KEY_F5,
    SPECIAL_KEY_F6,
    SPECIAL_KEY_F7,
    SPECIAL_KEY_F8,
    SPECIAL_KEY_F9,
    SPECIAL_KEY_F10,
    SPECIAL_KEY_F11,
    SPECIAL_KEY_F12,
    SPECIAL_KEY_F13,
    SPECIAL_KEY_F14,
    SPECIAL_KEY_F15,
    SPECIAL_KEY_F16,
    SPECIAL_KEY_F17,
    SPECIAL_KEY_F18,
    SPECIAL_KEY_F19,
    SPECIAL_KEY_F20,
    SPECIAL_KEY_F21,
    SPECIAL_KEY_F22,
    SPECIAL_KEY_F23,
    SPECIAL_KEY_F24,
    SPECIAL_KEY_F25,
    SPECIAL_KEY_F26,
    SPECIAL_KEY_F27,
    SPECIAL_KEY_F28,
    SPECIAL_KEY_F29,
    SPECIAL_KEY_F30,
    SPECIAL_KEY_F31,
    SPECIAL_KEY_F32,
    SPECIAL_KEY_F33,
    SPECIAL_KEY_F34,
    SPECIAL_KEY_F35,
    SPECIAL_KEY_AUDIO_VOL_DOWN,
    SPECIAL_KEY_AUDIO_VOL_MUTE,
    SPECIAL_KEY_AUDIO_VOL_UP,
    SPECIAL_KEY_AUDIO_PLAY,
    SPECIAL_KEY_AUDIO_STOP,
    SPECIAL_KEY_AUDIO_PREV,
    SPECIAL_KEY_AUDIO_NEXT,
    SPECIAL_KEY_LAST,
} SpecialKey;

typedef struct
{
    bool keep:1;
    bool close:1;
    bool inherit:1;
    bool ignore:1;
    bool ignoreSort:1;
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
    char* command;
    char* before;
    char* after;
    ChordFlags flags;
    struct KeyChord* keyChords;
} KeyChord;

void copyChordFlags(const ChordFlags* from, ChordFlags* to);
void copyChordModifiers(const Modifiers* from, Modifiers* to);
void copyKey(const Key* from, Key* to);
void copyKeyChord(const KeyChord* from, KeyChord* to);
uint32_t countChordFlags(const ChordFlags* flags);
uint32_t countKeyChords(const KeyChord* keyChords);
uint32_t countModifiers(const Modifiers* mods);
const char* getSpecialKeyLiteral(const SpecialKey special);
const char* getSpecialKeyRepr(const SpecialKey special);
bool hasChordFlags(const ChordFlags* flags);
void initKey(Key* key);
void initKeyChord(KeyChord* keyChord);
void initChordFlags(ChordFlags* flags);
void initChordModifiers(Modifiers* mods);
bool hasActiveModifier(const Modifiers* mods);
bool hasDefaultChordFlags(const ChordFlags* flags);
bool keysAreEqual(const Key* a, const Key* b, bool shiftIsSignificant);
void makeNullKeyChord(KeyChord* keyChord);

#endif /* WK_COMMON_KEY_CHORD_H_ */
