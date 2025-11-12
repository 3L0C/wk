#include <assert.h>
#include <stdbool.h>
#include <sysexits.h>

/* config includes */
#include "common/arena.h"
#include "common/array.h"

/* common includes */
#include "common/common.h"
#include "common/debug.h"
#include "common/menu.h"

/* compiler includes */
#include "compiler/common.h"
#include "compiler/compiler.h"
#include "compiler/preprocessor.h"
#include "compiler/writer.h"
#include "config/key_chords.h"

static Array
preprocessSource(Menu* menu, Array* source, const char* filepath)
{
    assert(menu), assert(source);

    Array processedSource = preprocessorRun(menu, source, filepath);
    if (arrayIsEmpty(&processedSource))
    {
        errorMsg("Failed while running preprocessor on `wks` file: '%s'.", filepath);
    }

    arrayFree(source);
    return processedSource;
}

static int
compileSource(Menu* menu, Compiler* compiler, Array* source, const char* filepath)
{
    assert(menu), assert(compiler), assert(source), assert(filepath);

    char* src = ARENA_ADOPT_ARRAY(&menu->arena, source, char);
    initCompiler(compiler, menu, src, filepath);

    /* Compile lines, retruns null on error. */
    menu->keyChordsHead = compileKeyChords(compiler, menu);
    if (!menu->keyChordsHead) return EX_DATAERR;

    return EX_OK;
}

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
        status = pressKeys(menu, menu->client.keys);
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

static int
runSource(Menu* menu, Array* source, const char* filepath)
{
    assert(menu), assert(source), assert(filepath);

    /* Run preprocessor on `script` and fail if mallformed or other error. */
    Array processedSource = preprocessSource(menu, source, filepath);
    if (arrayIsEmpty(&processedSource)) return EX_DATAERR;

    /* Begin compilation */
    Compiler compiler = { 0 };
    int      result   = compileSource(menu, &compiler, &processedSource, filepath);
    if (result != EX_OK)
    {
        errorMsg("Could not compile `wks` file: '%s'.", filepath);
        return result;
    }

    return runMenu(menu);
}

/* Read the given '.wks' file, and transpile it into chords.h syntax. */
static int
transpileWksFile(Menu* menu)
{
    assert(menu);

    /* Read given file to `source` and exit if read failed. */
    Array source = readFile(menu->client.transpile);
    if (arrayIsEmpty(&source)) return EX_IOERR;

    /* Run the preprocessor on source */
    Array processedSource = preprocessSource(menu, &source, menu->client.transpile);
    if (arrayIsEmpty(&processedSource)) return EX_DATAERR;

    Compiler compiler = { 0 };
    int      result   = compileSource(menu, &compiler, &processedSource, menu->client.transpile);
    if (result != EX_OK) return result;

    /* Well formed file, write to stdout. */
    writeBuiltinKeyChordsHeaderFile(menu->keyChordsHead);

    return result;
}

/* Read stdin as though it were a '.wks' file,
 * compile it into a Chord array, and execute like normal.
 */
static int
runScript(Menu* menu)
{
    assert(menu);

    int result = EX_SOFTWARE;

    /* Exit on failure to read stdin. */
    if (!menuTryStdin(menu)) return EX_IOERR;

    result = runSource(menu, &menu->client.script, ".");

    return result;
}

/* Read the given '.wks' file, compile it into a Chord array, and execute like normal. */
static int
runWksFile(Menu* menu)
{
    assert(menu);

    int result = EX_SOFTWARE;

    /* Exit on failure to read source file. */
    Array source = readFile(menu->client.wksFile);
    if (arrayIsEmpty(&source)) return EX_IOERR;

    result = runSource(menu, &source, menu->client.wksFile);

    return result;
}

/* Execute precompiled key chords. */
static int
runBuiltinKeyChords(Menu* menu)
{
    assert(menu);

    if (menu->debug) disassembleKeyChordArray(menu->keyChords, 0);
    return runMenu(menu);
}

int
main(int argc, char** argv)
{
    int result = EX_SOFTWARE;

    Menu menu = { 0 };
    menuInit(&menu, &builtinKeyChords);
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
