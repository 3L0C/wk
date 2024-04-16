#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "key_chord.h"
#include "memory.h"
#include "util.h"

void
copyKeyChordFlags(KeyChordFlags* from, KeyChordFlags* to)
{
    assert(from && to);

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

uint32_t
countKeyChordMods(const KeyChordMods* mods)
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
countKeyChordFlags(const KeyChordFlags* flags)
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
hasFlags(const KeyChordFlags* flags)
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

static void
resetKeyChordMods(KeyChordMods* mods)
{
    assert(mods);

    mods->ctrl = false;
    mods->alt = false;
    mods->hyper = false;
    mods->shift = false;
}

static void
resetKeyChordFlags(KeyChordFlags* flags)
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
initKeyChord(KeyChord* keyChord)
{
    assert(keyChord);

    keyChord->state = KEY_CHORD_STATE_NOT_NULL;
    resetKeyChordMods(&keyChord->mods);
    keyChord->special = SPECIAL_KEY_NONE;
    keyChord->key = NULL;
    keyChord->description = NULL;
    keyChord->hint = NULL;
    keyChord->command = NULL;
    keyChord->before = NULL;
    keyChord->after = NULL;
    resetKeyChordFlags(&keyChord->flags);
    keyChord->keyChords = NULL;
}

void
initKeyChordArray(KeyChordArray* dest, KeyChord** source)
{
    assert(dest && source && !*source);

    dest->keyChords = source;
    dest->count = 0;
    dest->capacity = 0;
}

bool
isKeyChordMod(const KeyChordMods* mods)
{
    assert(mods);

    return (mods->ctrl || mods->alt || mods->hyper || mods->shift);
}

bool
keyChordHasDefaultFlags(const KeyChordFlags* flags)
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

bool
keyIsNormal(const Key* key)
{
    return (!keyIsSpecial(key) && *key->key != '\0');
}

bool
keyIsSpecial(const Key* key)
{
    return (key->special != SPECIAL_KEY_NONE);
}

bool
keyIsStrictlyMod(const Key* key)
{
    return (isKeyChordMod(&key->mods) && !keyIsNormal(key) && !keyIsSpecial(key));
}

void
makePsuedoKeyChordArray(KeyChordArray* array, KeyChord** keyChords)
{
    assert(array && keyChords);

    array->keyChords = keyChords;
    if (!*keyChords)
    {
        array->count = 0;
        array->capacity = 0;
    }
    else
    {
        array->count = countKeyChords(*keyChords);
        array->capacity = array->count;
    }
}

static void
copyKeyChordMods(KeyChordMods* from, KeyChordMods* to)
{
    assert(from && to);

    to->ctrl = from->ctrl;
    to->alt = from->alt;
    to->hyper = from->hyper;
    to->shift = from->shift;
}

static void
copyKeyChord(KeyChord* from, KeyChord* to)
{
    assert(from && to);

    to->state = from->state;
    copyKeyChordMods(&from->mods, &to->mods);
    to->special = from->special;
    to->key = from->key;
    to->description = from->description;
    to->hint = from->hint;
    to->command = from->command;
    to->before = from->before;
    to->after = from->after;
    copyKeyChordFlags(&from->flags, &to->flags);
    to->keyChords = from->keyChords;
}

KeyChord*
writeKeyChordArray(KeyChordArray* array, KeyChord* keyChord)
{
    assert(array && keyChord);

    if (array->count == array->capacity)
    {
        size_t oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        *array->keyChords = GROW_ARRAY(KeyChord, *array->keyChords, oldCapacity, array->capacity);
    }

    copyKeyChord(keyChord, &(*array->keyChords)[array->count]);
    return &(*array->keyChords)[array->count++];
}
