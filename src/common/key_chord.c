#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* local includes */
#include "key_chord.h"

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

void
copyChordFlags(const ChordFlags* from, ChordFlags* to)
{
    assert(from), assert(to);

    to->keep = from->keep;
    to->close = from->close;
    to->inherit = from->inherit;
    to->ignore = from->ignore;
    to->unhook = from->unhook;
    to->deflag = from->deflag;
    to->nobefore = from->nobefore;
    to->noafter = from->noafter;
    to->write = from->write;
    to->execute = from->execute;
    to->syncCommand = from->syncCommand;
    to->syncBefore = from->syncBefore;
    to->syncAfter = from->syncAfter;
}

void
copyChordModifiers(const Modifiers* from, Modifiers* to)
{
    assert(from), assert(to);

    to->ctrl = from->ctrl;
    to->alt = from->alt;
    to->hyper = from->hyper;
    to->shift = from->shift;
}

void
copyKey(const Key* from, Key* to)
{
    assert(from), assert(to);

    copyChordModifiers(&from->mods, &to->mods);
    to->special = from->special;
    to->repr = from->repr;
    to->len = from->len;
}

void
copyKeyChord(const KeyChord* from, KeyChord* to)
{
    assert(from), assert(to);

    to->state = from->state;
    copyKey(&from->key, &to->key);
    to->description = from->description;
    to->hint = from->hint;
    to->command = from->command;
    to->before = from->before;
    to->after = from->after;
    copyChordFlags(&from->flags, &to->flags);
    to->keyChords = from->keyChords;
}

uint32_t
countModifiers(const Modifiers* mods)
{
    assert(mods);

    uint32_t result = 0;
    if (mods->ctrl) result++;
    if (mods->alt) result++;
    if (mods->hyper) result++;
    if (mods->shift) result++;

    return result;
}

uint32_t
countChordFlags(const ChordFlags* flags)
{
    assert(flags);

    uint32_t result = 0;

    if (flags->keep) result++;
    if (flags->close) result++;
    if (flags->inherit) result++;
    if (flags->ignore) result++;
    if (flags->unhook) result++;
    if (flags->deflag) result++;
    if (flags->nobefore) result++;
    if (flags->noafter) result++;
    if (flags->write) result++;
    if (flags->execute) result++;
    if (flags->syncCommand) result++;
    if (flags->syncBefore) result++;
    if (flags->syncAfter) result++;

    return result;
}

const char*
getSpecialKeyLiteral(const SpecialKey special)
{
    return specialTable[special].literal;
}

const char*
getSpecialKeyRepr(const SpecialKey special)
{
    return specialTable[special].repr;
}

bool
hasChordFlags(const ChordFlags* flags)
{
    assert(flags);

    return (
        flags->keep ||
        flags->close ||
        flags->inherit ||
        flags->ignore ||
        flags->unhook ||
        flags->deflag ||
        flags->nobefore ||
        flags->noafter ||
        flags->write ||
        flags->execute ||
        flags->syncCommand ||
        flags->syncBefore ||
        flags->syncAfter
    );
}

void
initKey(Key* key)
{
    assert(key);

    initChordModifiers(&key->mods);
    key->special = SPECIAL_KEY_NONE;
    key->repr = NULL;
    key->len = 0;
}

void
initKeyChord(KeyChord* keyChord)
{
    assert(keyChord);

    keyChord->state = KEY_CHORD_STATE_NOT_NULL;
    initKey(&keyChord->key);
    keyChord->description = NULL;
    keyChord->hint = NULL;
    keyChord->command = NULL;
    keyChord->before = NULL;
    keyChord->after = NULL;
    initChordFlags(&keyChord->flags);
    keyChord->keyChords = NULL;
}

void
initChordFlags(ChordFlags* flags)
{
    assert(flags);

    flags->keep = false;
    flags->close = false;
    flags->inherit = false;
    flags->ignore = false;
    flags->unhook = false;
    flags->deflag = false;
    flags->nobefore = false;
    flags->noafter = false;
    flags->write = false;
    flags->execute = false;
    flags->syncCommand = false;
    flags->syncBefore = false;
    flags->syncAfter = false;
}

void
initChordModifiers(Modifiers* mods)
{
    assert(mods);

    mods->ctrl = false;
    mods->alt = false;
    mods->hyper = false;
    mods->shift = false;
}

bool
hasActiveModifier(const Modifiers* mods)
{
    assert(mods);

    return (mods->ctrl || mods->alt || mods->hyper || mods->shift);
}

bool
hasDefaultChordFlags(const ChordFlags* flags)
{
    return (
        flags->keep == false &&
        flags->close == false &&
        flags->inherit == false &&
        flags->ignore == false &&
        flags->unhook == false &&
        flags->deflag == false &&
        flags->nobefore == false &&
        flags->noafter == false &&
        flags->write == false &&
        flags->execute == false &&
        flags->syncCommand == false &&
        flags->syncBefore == false &&
        flags->syncAfter == false
    );
}

static bool
modsAreEqual(const Modifiers* a, const Modifiers* b, bool checkShift)
{
    assert(a), assert(b);

    if (checkShift)
    {
        return (
            a->ctrl == b->ctrl &&
            a->alt == b->alt &&
            a->hyper == b->hyper &&
            a->shift == b->shift
        );
    }
    return (
        a->ctrl == b->ctrl &&
        a->alt == b->alt &&
        a->hyper == b->hyper
    );
}

static bool
keysAreSpecial(const Key* a, const Key* b)
{
    assert(a), assert(b);

    return a->special != SPECIAL_KEY_NONE && b->special != SPECIAL_KEY_NONE;
}

static bool
specialKeysAreEqual(const Key* a, const Key* b)
{
    assert(a), assert(b);

    return (
        a->special == b->special &&
        modsAreEqual(&a->mods, &b->mods, true)
    );
}

bool
keysAreEqual(const Key* a, const Key* b)
{
    assert(a), assert(b);

    if (keysAreSpecial(a, b) && specialKeysAreEqual(a, b)) return true;
    return (
        a->special == b->special &&
        modsAreEqual(&a->mods, &b->mods, false) &&
        strcmp(a->repr, b->repr) == 0
    );
}

void
makeNullKeyChord(KeyChord* keyChord)
{
    assert(keyChord);

    keyChord->state = KEY_CHORD_STATE_IS_NULL;
    initKey(&keyChord->key);
    keyChord->description = NULL;
    keyChord->hint = NULL;
    keyChord->command = NULL;
    keyChord->before = NULL;
    keyChord->after = NULL;
    initChordFlags(&keyChord->flags);
    keyChord->keyChords = NULL;
}
