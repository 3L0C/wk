#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

/* common includes */
#include "common/common.h"
#include "common/key_chord.h"

/* local includes */
#include "common.h"

bool
isNormalKey(Key* key, char* buffer, size_t len)
{
    assert(key), assert(buffer);
    if (!isUtf8MultiByteStartByte(*buffer) && iscntrl(*buffer)) return false;

    key->repr = buffer;
    key->len = len;

    return true;
}

bool
isSpecialKey(Key* key, void* keysym, GetSpecialKeyFp fp)
{
    assert(key), assert(keysym);

    key->special = fp(keysym);
    if (key->special == SPECIAL_KEY_NONE) return false;

    key->repr = (char*)getSpecialKeyRepr(key->special);
    key->len = strlen(key->repr);

    return true;
}
