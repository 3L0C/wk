#include <assert.h>
#include <stdbool.h>
#include <sysexits.h>

/* common includes */
#include "common/common.h"
#include "common/debug.h"
#include "common/menu.h"
#include "common/span.h"

/* compiler includes */
#include "compiler/compiler.h"
#include "compiler/writer.h"

static int
runMenu(Menu* menu)
{
    assert(menu);

    int        result = EX_SOFTWARE;
    MenuStatus status = MENU_STATUS_RUNNING;

    /* Pre-press keys */
    if (menu->client.keys)
    {
        if (menu->debug) debugMsg(true, "Trying to press key(s): '%s'.", menu->client.keys);
        status = menuHandlePath(menu, menu->client.keys);
    }

    /* If keys were pre-pressed there may be nothing to do, or an error to report. */
    if (status == MENU_STATUS_EXIT_SOFTWARE)
    {
        errorMsg("Key(s) not found in key chords: '%s'.", menu->client.keys);
        result = EX_DATAERR;
    }
    else if (status == MENU_STATUS_EXIT_OK)
    {
        debugMsg(menu->debug, "Successfully pressed keys: '%s'.", menu->client.keys);
        result = EX_OK;
    }
    else
    {
        result = menuDisplay(menu);
    }

    return result;
}

/* Read the given '.wks' file, and transpile it into chords.h syntax. */
static int
transpileWksFile(Menu* menu)
{
    assert(menu);

    menu->keyChordsHead = compile(menu, menu->client.transpile);
    if (!menu->keyChordsHead)
    {
        errorMsg("Could not compile `wks` file: '%s'.", menu->client.transpile);
        return EX_DATAERR;
    }

    writeConfigHeaderFile(menu->keyChordsHead, menu);
    return EX_OK;
}

/* Read stdin as though it were a '.wks' file,
 * compile it into a Chord array, and execute like normal.
 */
static int
runScript(Menu* menu)
{
    assert(menu);

    if (!menuTryStdin(menu)) return EX_IOERR;

    menu->keyChordsHead = compile(menu, NULL);
    if (!menu->keyChordsHead)
    {
        errorMsg("Could not compile script from stdin.");
        return EX_DATAERR;
    }

    if (menu->keyChords->count == 0)
    {
        errorMsg("No key chords found in stdin. Add at least one chord definition.");
        return EX_DATAERR;
    }

    return runMenu(menu);
}

/* Read the given '.wks' file, compile it into a Chord array, and execute like normal. */
static int
runWksFile(Menu* menu)
{
    assert(menu);

    /* Use new compiler entry point */
    menu->keyChordsHead = compile(menu, menu->client.wksFile);
    if (!menu->keyChordsHead)
    {
        errorMsg("Could not compile `wks` file: '%s'.", menu->client.wksFile);
        return EX_DATAERR;
    }

    if (menu->keyChords->count == 0)
    {
        errorMsg("No key chords found in '%s'. Add at least one chord definition.", menu->client.wksFile);
        return EX_DATAERR;
    }

    return runMenu(menu);
}

/* Execute precompiled key chords. */
static int
runBuiltinKeyChords(Menu* menu)
{
    assert(menu);

    if (menu->debug) disassembleKeyChordSpan(menu->keyChords, 0);
    return runMenu(menu);
}

int
main(int argc, char** argv)
{
    int result = EX_SOFTWARE;

    Menu menu = { 0 };
    menuInit(&menu);
    menuParseArgs(&menu, &argc, &argv);

    if (menu.debug) disassembleMenu(&menu);

    if (menu.client.transpile)
    {
        result = transpileWksFile(&menu);
    }
    else if (menu.client.tryScript)
    {
        result = runScript(&menu);
    }
    else if (menu.client.wksFile)
    {
        result = runWksFile(&menu);
    }
    else
    {
        result = runBuiltinKeyChords(&menu);
    }

    menuFree(&menu);
    return result;
}
