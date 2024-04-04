#include "types.h"

bool
keyIsNormal(Key* key)
{
    return (!keyIsSpecial(key) && *key->key != '\0');
}

bool
keyIsSpecial(Key* key)
{
    return (key->special != WK_SPECIAL_NONE);
}

bool
keyIsStrictlyMod(Key* key)
{
    return (IS_MOD(key->mods) && !keyIsNormal(key) && !keyIsSpecial(key));
    return (IS_MOD(key->mods) && *key->key == '\0' && key->special == WK_SPECIAL_NONE);
}
