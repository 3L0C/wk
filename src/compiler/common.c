#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

/* common includes */
#include "common/arena.h"
#include "common/common.h"
#include "common/key_chord.h"
#include "common/string.h"
#include "common/vector.h"

/* local includes */
#include "common.h"
#include "scanner.h"
#include "token.h"

static bool
isMod(TokenType type)
{
    switch (type)
    {
    case TOKEN_MOD_CTRL:
    case TOKEN_MOD_META:
    case TOKEN_MOD_HYPER:
    case TOKEN_MOD_SHIFT: return true;
    default: return false;
    }
}

static void
addMod(Key* key, TokenType type)
{
    assert(key);

    switch (type)
    {
    case TOKEN_MOD_CTRL: key->mods |= MOD_CTRL; break;
    case TOKEN_MOD_META: key->mods |= MOD_META; break;
    case TOKEN_MOD_HYPER: key->mods |= MOD_HYPER; break;
    case TOKEN_MOD_SHIFT: key->mods |= MOD_SHIFT; break;
    default: break;
    }
}

bool
compileKeys(Arena* arena, const char* keys, Vector* outKeys)
{
    assert(arena), assert(keys), assert(outKeys);

    Scanner scanner;
    scannerInit(&scanner, keys, "KEYS");

    while (!scannerIsAtEnd(&scanner))
    {
        Key key = { 0 };
        keyInit(&key);

        Token token = { 0 };
        tokenInit(&token);
        scannerTokenForCompiler(&scanner, &token);

        while (isMod(token.type))
        {
            addMod(&key, token.type);
            scannerTokenForCompiler(&scanner, &token);
        }

        if (token.type == TOKEN_KEY)
        {
            key.special = SPECIAL_KEY_NONE;
            key.repr    = stringMake(arena, token.start, token.length);
            vectorAppend(outKeys, &key);
        }
        else if (token.type == TOKEN_SPECIAL_KEY)
        {
            key.special = token.special;
            key.repr    = stringFromCString(arena, specialKeyRepr(key.special));
            vectorAppend(outKeys, &key);
        }
        else if (token.type == TOKEN_EOF)
        {
            keyFree(&key);
            break;
        }
        else
        {
            errorMsg(
                "Key does not appear to be a regular key or a special key: '%.*s'.",
                (int)token.length,
                token.start);
            keyFree(&key);
            return false;
        }
    }

    return true;
}
