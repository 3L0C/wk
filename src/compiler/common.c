#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* common includes */
#include "common/common.h"
#include "common/debug.h"
#include "common/key_chord.h"
#include "common/util.h"

/* local includes */
#include "common.h"
#include "scanner.h"
#include "token.h"

static bool
addMod(Key* key, TokenType type)
{
    assert(key);

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

static MenuStatus
pressKey(Menu* menu, Scanner* scanner)
{
    assert(menu), assert(scanner);

    static const size_t bufmax = 128;
    char buffer[bufmax];
    memset(buffer, 0, 32);
    Key key = {0};
    Token token = {0};
    initToken(&token);
    scanTokenForCompiler(scanner, &token);

    while (addMod(&key, token.type))
    {
        scanTokenForCompiler(scanner, &token);
    }

    if (token.type == TOKEN_KEY)
    {
        key.special = SPECIAL_KEY_NONE;
        if (token.length > bufmax)
        {
            errorMsg("Key is longer than max size of %zu: %04zu", bufmax, token.length);
            return MENU_STATUS_EXIT_SOFTWARE;
        }
        memcpy(buffer, token.start, token.length);
        buffer[token.length] = '\0';
        key.repr = buffer;
        key.len = token.length;
    }
    else if (token.type == TOKEN_SPECIAL_KEY)
    {
        key.repr = NULL;
        key.special = scanner->special;
    }
    else
    {
        errorMsg(
            "Key does not appear to be a regular key or a special key: '%.*s'.",
            (int)token.length, token.start
        );
        return MENU_STATUS_EXIT_SOFTWARE;
    }

    if (menu->debug)
    {
        debugMsg(menu->debug, "Trying to press key: '%.*s'.", (int)token.length, token.start);
        disassembleKey(&key);
    }

    MenuStatus status = handleKeypress(menu, &key);

    if (status == MENU_STATUS_EXIT_SOFTWARE)
    {
        errorMsg("Could not press key: '%.*s'.", (int)token.length, token.start);
        return status;
    }

    if (status == MENU_STATUS_EXIT_OK)
    {
        scanTokenForCompiler(scanner, &token);
        if (token.type == TOKEN_EOF) return status;
        return MENU_STATUS_EXIT_SOFTWARE;
    }

    return status;
}

MenuStatus
pressKeys(Menu* menu, const char* keys)
{
    assert(menu), assert(keys);

    Scanner scanner;
    initScanner(&scanner, keys, "KEYS");
    MenuStatus status = pressKey(menu, &scanner);

    while (!isAtEnd(&scanner) && statusIsRunning(status))
    {
        status = pressKey(menu, &scanner);
    }

    if (status == MENU_STATUS_EXIT_OK && *scanner.current != '\0')
    {
        errorMsg(
            "Reached end of chords but not end of keys: '%s'.",
            (scanner.current == scanner.start) ? scanner.start : scanner.current - 1
        );
        return status;
    }

    return status;
}
