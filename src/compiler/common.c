#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

/* common includes */
#include "common/common.h"
#include "common/debug.h"
#include "common/key_chord.h"

/* local includes */
#include "common.h"
#include "common/menu.h"
#include "common/string.h"
#include "scanner.h"
#include "token.h"

static bool
addMod(Key* key, TokenType type)
{
    assert(key);

    switch (type)
    {
    case TOKEN_MOD_CTRL: key->mods  |= MOD_CTRL; break;
    case TOKEN_MOD_META: key->mods   |= MOD_META; break;
    case TOKEN_MOD_HYPER: key->mods |= MOD_HYPER; break;
    case TOKEN_MOD_SHIFT: key->mods |= MOD_SHIFT; break;
    default: return false;
    }

    return true;
}

static MenuStatus
pressKey(Menu* menu, Scanner* scanner)
{
    assert(menu), assert(scanner);

    Key key = {0};
    keyInit(&key);
    Token token = {0};
    tokenInit(&token);
    scannerGetTokenForCompiler(scanner, &token);

    while (addMod(&key, token.type))
    {
        scannerGetTokenForCompiler(scanner, &token);
    }

    if (token.type == TOKEN_KEY)
    {
        key.special = SPECIAL_KEY_NONE;
        stringAppend(&key.repr, token.start, token.length);
    }
    else if (token.type == TOKEN_SPECIAL_KEY)
    {
        key.special = token.special;
        stringAppendCString(&key.repr, specialKeyGetRepr(key.special));
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

    MenuStatus status = menuHandleKeypress(menu, &key);
    keyFree(&key);

    if (status == MENU_STATUS_EXIT_SOFTWARE)
    {
        errorMsg("Could not press key: '%.*s'.", (int)token.length, token.start);
        return status;
    }

    if (status == MENU_STATUS_EXIT_OK)
    {
        scannerGetTokenForCompiler(scanner, &token);
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
    scannerInit(&scanner, keys, "KEYS");
    MenuStatus status = pressKey(menu, &scanner);

    while (!scannerIsAtEnd(&scanner) && menuStatusIsRunning(status))
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
