#ifndef WK_COMMON_KEY_CHORD_H_
#define WK_COMMON_KEY_CHORD_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Common includes */
#include "array.h"
#include "key_chord_def.h"
#include "property.h"
#include "string.h"

typedef uint16_t ChordFlag;
enum
{
    FLAG_NONE         = 0,
    FLAG_KEEP         = 1 << 0,
    FLAG_CLOSE        = 1 << 1,
    FLAG_INHERIT      = 1 << 2,
    FLAG_IGNORE       = 1 << 3,
    FLAG_IGNORE_SORT  = 1 << 4,
    FLAG_UNHOOK       = 1 << 5,
    FLAG_DEFLAG       = 1 << 6,
    FLAG_NO_BEFORE    = 1 << 7,
    FLAG_NO_AFTER     = 1 << 8,
    FLAG_WRITE        = 1 << 9,
    FLAG_EXECUTE      = 1 << 10,
    FLAG_SYNC_COMMAND = 1 << 11,
    FLAG_SYNC_BEFORE  = 1 << 12,
    FLAG_SYNC_AFTER   = 1 << 13,
    FLAG_UNWRAP       = 1 << 14,
};

typedef uint8_t KeyType;
enum
{
    KT_STRICTLY_MOD,
    KT_SPECIAL,
    KT_NORMAL,
    KT_UNKNOWN,
};

typedef uint8_t Modifier;
enum
{
    MOD_NONE  = 0,
    MOD_CTRL  = 1 << 0,
    MOD_META  = 1 << 1,
    MOD_HYPER = 1 << 2,
    MOD_SHIFT = 1 << 3
};

typedef uint8_t SpecialKey;
enum
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
    SPECIAL_KEY_BS,
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
};

typedef struct
{
    String     repr;
    Modifier   mods;
    SpecialKey special;
} Key;

/* KeyChord property slot IDs - generated from KEY_CHORD_PROP_LIST */
typedef enum
{
#define KC_PROP(id, name, accessor, rt, ct) id,
    KEY_CHORD_PROP_LIST
#undef KC_PROP
        KC_PROP_COUNT
} KeyChordPropId;

typedef struct KeyChord
{
    Key       key;
    Property  props[KC_PROP_COUNT];
    ChordFlag flags;
    Array     keyChords;
} KeyChord;

/* Helpers */
int       chordFlagCount(ChordFlag flag);
bool      chordFlagHasAnyActive(ChordFlag flag);
ChordFlag chordFlagInit(void);
bool      chordFlagIsActive(ChordFlag flag, ChordFlag test);
bool      chordFlagsAreDefault(ChordFlag flag);

int      modifierCount(Modifier mod);
bool     modifierHasAnyActive(Modifier mod);
Modifier modifierInit(void);
bool     modifierIsActive(Modifier mod, Modifier test);

const char* specialKeyGetLiteral(const SpecialKey special);
const char* specialKeyGetRepr(const SpecialKey special);

/* Core */
void keyCopy(const Key* from, Key* to);
void keyFree(Key* key);
void keyInit(Key* key);
bool keyIsEqual(const Key* a, const Key* b);

void keyChordsFree(Array* keyChords);
void keyChordCopy(const KeyChord* from, KeyChord* to);
void keyChordFree(KeyChord* keyChord);
void keyChordInit(KeyChord* keyChord);

/* Property slot access */
Property*       keyChordProperty(KeyChord* chord, KeyChordPropId id);
const Property* keyChordPropertyConst(const KeyChord* chord, KeyChordPropId id);
PropertyType    keyChordCompileType(KeyChordPropId id);
PropertyType    keyChordRuntimeType(KeyChordPropId id);

/* Type-specific slot accessors - return NULL if prop->type doesn't match */
#define PROP_TYPE_X(name, ctype, accessor, field)                        \
    ctype*       keyChord##accessor(KeyChord* chord, KeyChordPropId id); \
    const ctype* keyChord##accessor##Const(const KeyChord* chord, KeyChordPropId id);
PROPERTY_TYPE_LIST
#undef PROP_TYPE_X

/* Query helpers */
bool keyChordIsSet(const KeyChord* chord, KeyChordPropId id);

/* Generated typed accessors - declared here, defined in key_chord.c */
#define KC_PROP(id, name, accessor, rt, ct)            \
    String*       keyChord##accessor(KeyChord* chord); \
    const String* keyChord##accessor##Const(const KeyChord* chord);
KEY_CHORD_PROP_LIST
#undef KC_PROP

#endif /* WK_COMMON_KEY_CHORD_H_ */
