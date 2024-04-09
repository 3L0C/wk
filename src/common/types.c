#include "types.h"

bool
keyIsNormal(WkKey* key)
{
    return (!keyIsSpecial(key) && *key->key != '\0');
}

bool
keyIsSpecial(WkKey* key)
{
    return (key->special != WK_SPECIAL_NONE);
}

bool
keyIsStrictlyMod(WkKey* key)
{
    return (IS_MOD(key->mods) && !keyIsNormal(key) && !keyIsSpecial(key));
    return (IS_MOD(key->mods) && *key->key == '\0' && key->special == WK_SPECIAL_NONE);
}
