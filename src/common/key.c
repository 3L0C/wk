#include <assert.h>
#include <stdbool.h>

/* local includes */
#include "key.h"
#include "string.h"

static bool
modifiersAreEqual(Modifier a, Modifier b)
{
    return a == b;
}

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
        modifiersAreEqual(a->mods, b->mods));
}

void
keyCopy(const Key* from, Key* to)
{
    assert(from), assert(to);

    to->mods |= from->mods;
    to->special = from->special;
    to->repr    = from->repr;
}

void
keyFree(Key* key)
{
    assert(key);
    (void)key;
}

void
keyInit(Key* key)
{
    assert(key);

    key->repr    = (String){ 0 };
    key->mods    = modifierInit();
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
        stringEquals(&a->repr, &b->repr));
}
