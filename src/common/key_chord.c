#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* local includes */
#include "key_chord.h"
#include "array.h"
#include "string.h"

/* Helpers */
typedef struct
{
    const char* literal;
    const char* repr;
} SpecialTable;

static const SpecialTable specialTable[SPECIAL_KEY_LAST] = {
    [SPECIAL_KEY_NONE] = { "SPECIAL_KEY_NONE", "None" },
    [SPECIAL_KEY_LEFT] = { "SPECIAL_KEY_LEFT", "Left" },
    [SPECIAL_KEY_RIGHT] = { "SPECIAL_KEY_RIGHT", "Right" },
    [SPECIAL_KEY_UP] = { "SPECIAL_KEY_UP", "Up" },
    [SPECIAL_KEY_DOWN] = { "SPECIAL_KEY_DOWN", "Down" },
    [SPECIAL_KEY_TAB] = { "SPECIAL_KEY_TAB", "TAB" },
    [SPECIAL_KEY_SPACE] = { "SPECIAL_KEY_SPACE", "SPC" },
    [SPECIAL_KEY_RETURN] = { "SPECIAL_KEY_RETURN", "RET" },
    [SPECIAL_KEY_DELETE] = { "SPECIAL_KEY_DELETE", "DEL" },
    [SPECIAL_KEY_ESCAPE] = { "SPECIAL_KEY_ESCAPE", "ESC" },
    [SPECIAL_KEY_HOME] = { "SPECIAL_KEY_HOME", "Home" },
    [SPECIAL_KEY_PAGE_UP] = { "SPECIAL_KEY_PAGE_UP", "PgUp" },
    [SPECIAL_KEY_PAGE_DOWN] = { "SPECIAL_KEY_PAGE_DOWN", "PgDown" },
    [SPECIAL_KEY_END] = { "SPECIAL_KEY_END", "End" },
    [SPECIAL_KEY_BEGIN] = { "SPECIAL_KEY_BEGIN", "Begin" },
    [SPECIAL_KEY_F1] = { "SPECIAL_KEY_F1", "F1" },
    [SPECIAL_KEY_F2] = { "SPECIAL_KEY_F2", "F2" },
    [SPECIAL_KEY_F3] = { "SPECIAL_KEY_F3", "F3" },
    [SPECIAL_KEY_F4] = { "SPECIAL_KEY_F4", "F4" },
    [SPECIAL_KEY_F5] = { "SPECIAL_KEY_F5", "F5" },
    [SPECIAL_KEY_F6] = { "SPECIAL_KEY_F6", "F6" },
    [SPECIAL_KEY_F7] = { "SPECIAL_KEY_F7", "F7" },
    [SPECIAL_KEY_F8] = { "SPECIAL_KEY_F8", "F8" },
    [SPECIAL_KEY_F9] = { "SPECIAL_KEY_F9", "F9" },
    [SPECIAL_KEY_F10] = { "SPECIAL_KEY_F10", "F10" },
    [SPECIAL_KEY_F11] = { "SPECIAL_KEY_F11", "F11" },
    [SPECIAL_KEY_F12] = { "SPECIAL_KEY_F12", "F12" },
    [SPECIAL_KEY_F13] = { "SPECIAL_KEY_F13", "F13" },
    [SPECIAL_KEY_F14] = { "SPECIAL_KEY_F14", "F14" },
    [SPECIAL_KEY_F15] = { "SPECIAL_KEY_F15", "F15" },
    [SPECIAL_KEY_F16] = { "SPECIAL_KEY_F16", "F16" },
    [SPECIAL_KEY_F17] = { "SPECIAL_KEY_F17", "F17" },
    [SPECIAL_KEY_F18] = { "SPECIAL_KEY_F18", "F18" },
    [SPECIAL_KEY_F19] = { "SPECIAL_KEY_F19", "F19" },
    [SPECIAL_KEY_F20] = { "SPECIAL_KEY_F20", "F20" },
    [SPECIAL_KEY_F21] = { "SPECIAL_KEY_F21", "F21" },
    [SPECIAL_KEY_F22] = { "SPECIAL_KEY_F22", "F22" },
    [SPECIAL_KEY_F23] = { "SPECIAL_KEY_F23", "F23" },
    [SPECIAL_KEY_F24] = { "SPECIAL_KEY_F24", "F24" },
    [SPECIAL_KEY_F25] = { "SPECIAL_KEY_F25", "F25" },
    [SPECIAL_KEY_F26] = { "SPECIAL_KEY_F26", "F26" },
    [SPECIAL_KEY_F27] = { "SPECIAL_KEY_F27", "F27" },
    [SPECIAL_KEY_F28] = { "SPECIAL_KEY_F28", "F28" },
    [SPECIAL_KEY_F29] = { "SPECIAL_KEY_F29", "F29" },
    [SPECIAL_KEY_F30] = { "SPECIAL_KEY_F30", "F30" },
    [SPECIAL_KEY_F31] = { "SPECIAL_KEY_F31", "F31" },
    [SPECIAL_KEY_F32] = { "SPECIAL_KEY_F32", "F32" },
    [SPECIAL_KEY_F33] = { "SPECIAL_KEY_F33", "F33" },
    [SPECIAL_KEY_F34] = { "SPECIAL_KEY_F34", "F34" },
    [SPECIAL_KEY_F35] = { "SPECIAL_KEY_F35", "F35" },
    [SPECIAL_KEY_AUDIO_VOL_DOWN] = { "SPECIAL_KEY_AUDIO_VOL_DOWN", "VolDown" },
    [SPECIAL_KEY_AUDIO_VOL_MUTE] = { "SPECIAL_KEY_AUDIO_VOL_MUTE", "VolMute" },
    [SPECIAL_KEY_AUDIO_VOL_UP] = { "SPECIAL_KEY_AUDIO_VOL_UP", "VolUp" },
    [SPECIAL_KEY_AUDIO_PLAY] = { "SPECIAL_KEY_AUDIO_PLAY", "Play" },
    [SPECIAL_KEY_AUDIO_STOP] = { "SPECIAL_KEY_AUDIO_STOP", "Stop" },
    [SPECIAL_KEY_AUDIO_PREV] = { "SPECIAL_KEY_AUDIO_PREV", "Prev" },
    [SPECIAL_KEY_AUDIO_NEXT] = { "SPECIAL_KEY_AUDIO_NEXT", "Next" },
};

int
chordFlagCount(ChordFlag flag)
{
    int result = 0;
    while (flag)
    {
        result++;
        flag &= flag - 1;
    }

    return result;
}

bool
chordFlagHasAnyActive(ChordFlag flag)
{
    static const ChordFlag any = FLAG_KEEP
        | FLAG_CLOSE
        | FLAG_INHERIT
        | FLAG_IGNORE
        | FLAG_IGNORE_SORT
        | FLAG_UNHOOK
        | FLAG_DEFLAG
        | FLAG_NO_BEFORE
        | FLAG_NO_AFTER
        | FLAG_WRITE
        | FLAG_EXECUTE
        | FLAG_SYNC_COMMAND
        | FLAG_SYNC_BEFORE
        | FLAG_SYNC_AFTER;

    return (flag & any) != 0;
}

bool
chordFlagsAreDefault(ChordFlag flag)
{
    return !chordFlagHasAnyActive(flag);
}

ChordFlag
chordFlagInit(void)
{
    return FLAG_NONE;
}

bool
chordFlagIsActive(ChordFlag flag, ChordFlag test)
{
    return (flag & test) != 0;
}

static bool
modifiersAreEqual(Modifier a, Modifier b)
{
    return a == b;
}

int
modifierCount(Modifier mod)
{
    int result = 0;
    while (mod)
    {
        result++;
        mod &= mod - 1;
    }

    return result;
}

bool
modifierHasAnyActive(Modifier mod)
{
    static const Modifier any = MOD_CTRL
        | MOD_META
        | MOD_HYPER
        | MOD_SHIFT;

    return (mod & any) != 0;
}

Modifier
modifierInit(void)
{
    return MOD_NONE;
}

bool
modifierIsActive(Modifier mod, Modifier test)
{
    return (mod & test) != 0;
}


const char*
specialKeyGetLiteral(const SpecialKey special)
{
    return specialTable[special].literal;
}

const char*
specialKeyGetRepr(const SpecialKey special)
{
    return specialTable[special].repr;
}

/* Core */
static bool
keyIsSpecial(const Key* key)
{
    assert(key);

    return key->special != SPECIAL_KEY_NONE;
}

static bool
keyIsEqualSpecial(const Key* a, const Key* b)
{
    assert(a), assert(b);

    if (!keyIsSpecial(a) || !keyIsSpecial(b)) return false;

    return (
        a->special == b->special &&
        modifiersAreEqual(a->mods, b->mods)
    );
}

void
keyCopy(const Key* from, Key* to)
{
    assert(from), assert(to);

    to->mods |= from->mods;
    to->special = from->special;
    to->repr = stringCopy(&from->repr);
}

void
keyFree(Key* key)
{
    assert(key);

    stringFree(&key->repr);
}

void
keyInit(Key* key)
{
    assert(key);

    key->repr = stringInit();
    key->mods = modifierInit();
    key->special = SPECIAL_KEY_NONE;
}

bool
keyIsEqual(const Key* a, const Key* b)
{
    assert(a), assert(b);

    if (keyIsEqualSpecial(a, b)) return true;
    return (
        a->special == b->special &&
        modifiersAreEqual(a->mods, b->mods) &&
        stringEquals(&a->repr, &b->repr)
    );
}

void
keyChordArrayFree(Array* keyChords)
{
    assert(keyChords);

    forEach(keyChords, KeyChord, keyChord)
    {
        keyChordFree(keyChord);
    }
    arrayFree(keyChords);
}

void
keyChordCopy(const KeyChord* from, KeyChord* to)
{
    assert(from), assert(to);

    keyCopy(&from->key, &to->key);
    to->description = from->description;
    to->command = from->command;
    to->before = from->before;
    to->after = from->after;
    to->flags = from->flags;
    to->keyChords = from->keyChords;
}

void
keyChordFree(KeyChord* keyChord)
{
    assert(keyChord);

    stringFree(&keyChord->key.repr);
    stringFree(&keyChord->description);
    stringFree(&keyChord->command);
    stringFree(&keyChord->before);
    stringFree(&keyChord->after);
    keyChordArrayFree(&keyChord->keyChords);
}

void
keyChordInit(KeyChord* keyChord)
{
    assert(keyChord);

    keyInit(&keyChord->key);
    keyChord->description = stringInit();
    keyChord->command = stringInit();
    keyChord->before = stringInit();
    keyChord->after = stringInit();
    keyChord->flags = chordFlagInit();
    keyChord->keyChords = ARRAY_INIT(KeyChord);
}

