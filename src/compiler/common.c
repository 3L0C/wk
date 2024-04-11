#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* common includes */
#include "common/common.h"
#include "common/debug.h"
#include "common/types.h"
#include "common/util.h"

/* local includes */
#include "common.h"
#include "scanner.h"
#include "token.h"

static bool
addMod(WkKey* key, TokenType type)
{
    switch (type)
    {
    case TOKEN_MOD_CTRL: key->mods.ctrl = true; break;
    case TOKEN_MOD_ALT: key->mods.alt = true; break;
    case TOKEN_MOD_HYPER: key->mods.hyper = true; break;
    case TOKEN_MOD_SHIFT: key->mods.shift = true; break;
    default: return false;
    }

    return true;
}

static bool
addSpecial(WkKey* key, TokenType type)
{
    key->key = NULL;

    switch (type)
    {
    case TOKEN_SPECIAL_LEFT: key->special = WK_SPECIAL_LEFT; break;
    case TOKEN_SPECIAL_RIGHT: key->special = WK_SPECIAL_RIGHT; break;
    case TOKEN_SPECIAL_UP: key->special = WK_SPECIAL_UP; break;
    case TOKEN_SPECIAL_DOWN: key->special = WK_SPECIAL_DOWN; break;
    case TOKEN_SPECIAL_TAB: key->special = WK_SPECIAL_TAB; break;
    case TOKEN_SPECIAL_SPACE: key->special = WK_SPECIAL_SPACE; break;
    case TOKEN_SPECIAL_RETURN: key->special = WK_SPECIAL_RETURN; break;
    case TOKEN_SPECIAL_DELETE: key->special = WK_SPECIAL_DELETE; break;
    case TOKEN_SPECIAL_ESCAPE: key->special = WK_SPECIAL_ESCAPE; break;
    case TOKEN_SPECIAL_HOME: key->special = WK_SPECIAL_HOME; break;
    case TOKEN_SPECIAL_PAGE_UP: key->special = WK_SPECIAL_PAGE_UP; break;
    case TOKEN_SPECIAL_PAGE_DOWN: key->special = WK_SPECIAL_PAGE_DOWN; break;
    case TOKEN_SPECIAL_END: key->special = WK_SPECIAL_END; break;
    case TOKEN_SPECIAL_BEGIN: key->special = WK_SPECIAL_BEGIN; break;
    default: return false;
    }

    return true;
}

static WkStatus
pressKey(WkMenu* menu, Scanner* scanner)
{
    assert(menu && scanner);

    static const size_t bufmax = 32;
    char buffer[bufmax];
    memset(buffer, 0, 32);
    WkKey key = {0};
    Token token = {0};
    initToken(&token);
    scanToken(scanner, &token);

    while (addMod(&key, token.type))
    {
        scanToken(scanner, &token);
    }

    if (token.type == TOKEN_KEY)
    {
        key.special = WK_SPECIAL_NONE;
        if (token.length > bufmax)
        {
            errorMsg("Key is longer than max size of %zu: %04zu", bufmax, token.length);
            return WK_STATUS_EXIT_SOFTWARE;
        }
        memcpy(buffer, token.start, token.length);
        buffer[token.length] = '\0';
        key.key = buffer;
        key.len = token.length;
    }
    else if (!addSpecial(&key, token.type))
    {
        errorMsg(
            "Key does not appear to be a regular key or a special key: '%.*s'.",
            (int)token.length, token.start
        );
        return WK_STATUS_EXIT_SOFTWARE;
    }

    if (menu->debug)
    {
        debugMsg(menu->debug, "Trying to press key: '%.*s'.", (int)token.length, token.start);
        debugKey(&key);
    }

    WkStatus status = handleKeypress(menu, &key);

    if (status == WK_STATUS_EXIT_SOFTWARE)
    {
        errorMsg("Could not press key: '%.*s'.", (int)token.length, token.start);
        return status;
    }

    if (status == WK_STATUS_EXIT_OK)
    {
        scanToken(scanner, &token);
        if (token.type == TOKEN_EOF) return status;
        return WK_STATUS_EXIT_SOFTWARE;
    }

    return status;
}

WkStatus
pressKeys(WkMenu* menu, const char* keys)
{
    assert(menu && keys);

    Scanner scanner;
    initScanner(&scanner, keys, "KEYS");
    WkStatus status = pressKey(menu, &scanner);

    while (!isAtEnd(&scanner) && statusIsRunning(status))
    {
        status = pressKey(menu, &scanner);
    }

    if (status == WK_STATUS_EXIT_OK && *scanner.current != '\0')
    {
        errorMsg(
            "Reached end of chords but not end of keys: '%s'.",
            (scanner.current == scanner.start) ? scanner.start : scanner.current - 1
        );
        return status;
    }

    return status;
}
