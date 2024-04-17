#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "key_chord.h"
/* #include "memory.h" */
/* #include "util.h" */

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
getSpecialKeyRepr(const SpecialKey special)
{
    switch (special)
    {
    case SPECIAL_KEY_NONE: return "SPECIAL_KEY_NONE";
    case SPECIAL_KEY_LEFT: return "SPECIAL_KEY_LEFT";
    case SPECIAL_KEY_RIGHT: return "SPECIAL_KEY_RIGHT";
    case SPECIAL_KEY_UP: return "SPECIAL_KEY_UP";
    case SPECIAL_KEY_DOWN: return "SPECIAL_KEY_DOWN";
    case SPECIAL_KEY_TAB: return "SPECIAL_KEY_TAB";
    case SPECIAL_KEY_SPACE: return "SPECIAL_KEY_SPACE";
    case SPECIAL_KEY_RETURN: return "SPECIAL_KEY_RETURN";
    case SPECIAL_KEY_DELETE: return "SPECIAL_KEY_DELETE";
    case SPECIAL_KEY_ESCAPE: return "SPECIAL_KEY_ESCAPE";
    case SPECIAL_KEY_HOME: return "SPECIAL_KEY_HOME";
    case SPECIAL_KEY_PAGE_UP: return "SPECIAL_KEY_PAGE_UP";
    case SPECIAL_KEY_PAGE_DOWN: return "SPECIAL_KEY_PAGE_DOWN";
    case SPECIAL_KEY_END: return "SPECIAL_KEY_END";
    case SPECIAL_KEY_BEGIN: return "SPECIAL_KEY_BEGIN";
    default: return "SPECIAL_KEY_UNKNOWN";
    }
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

bool
keyIsNormal(const Key* key)
{
    assert(key);

    return (!keyIsSpecial(key) && *key->repr != '\0');
}

bool
keyIsSpecial(const Key* key)
{
    assert(key);

    return (key->special != SPECIAL_KEY_NONE);
}

bool
keyIsStrictlyMod(const Key* key)
{
    assert(key);

    return (hasActiveModifier(&key->mods) && !keyIsNormal(key) && !keyIsSpecial(key));
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
