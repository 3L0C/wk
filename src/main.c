#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

/* config includes */
#include "config/key_chords.h"

/* common includes */
#include "common/common.h"
#include "common/debug.h"
#include "common/menu.h"
#include "common/string.h"
#include "common/key_chord.h"

/* compiler includes */
#include "compiler/common.h"
#include "compiler/compiler.h"
#include "compiler/preprocessor.h"
#include "compiler/writer.h"

static void
freeKeyChords(KeyChord* keyChords)
{
    if (!keyChords) return;

    for (size_t i = 0; keyChords[i].state == KEY_CHORD_STATE_NOT_NULL; i++)
    {
        free(keyChords[i].key.repr);
        free(keyChords[i].description);
        free(keyChords[i].hint);
        free(keyChords[i].command);
        free(keyChords[i].before);
        free(keyChords[i].after);
        if (keyChords[i].keyChords) freeKeyChords(keyChords[i].keyChords);
    }
    free(keyChords);
}

static void
freeMenuGarbage(Menu* menu)
{
    if (menu->garbage.shell) free(menu->garbage.shell);
    if (menu->garbage.font) free(menu->garbage.font);
    if (menu->garbage.foregroundColor) free(menu->garbage.foregroundColor);
    if (menu->garbage.backgroundColor) free(menu->garbage.backgroundColor);
    if (menu->garbage.borderColor) free(menu->garbage.borderColor);
}

static char*
preprocessSource(Menu* menu, const char* source, const char* filepath)
{
    assert(menu), assert(source);

    char* processedSource = runPreprocessor(menu, source, filepath);
    if (!processedSource)
    {
        errorMsg("Failed while running preprocessor on `wks` file: '%s'.", filepath);
    }
    else if (menu->debug)
    {
        debugPrintHeader(" Contents of Preprocessed Source ");
        debugMsg(true, "| ");
        debugTextWithLineNumber(processedSource);
        debugMsg(true, "| ");
        debugPrintHeader("");
    }

    return processedSource;
}

static int
compileSource(Menu* menu, Compiler* compiler, char* source, const char* filepath)
{
    assert(menu),assert(compiler), assert(source), assert(filepath);

    initCompiler(menu, compiler, source, filepath);

    /* Compile lines, retruns null on error. */
    menu->keyChordsHead = menu->keyChords = compileKeyChords(compiler, menu);
    if (!menu->keyChords) return EX_DATAERR;

    return EX_OK;
}

static int
runMenu(Menu* menu)
{
    assert(menu);

    int result = EX_SOFTWARE;
    MenuStatus status = MENU_STATUS_RUNNING;
    countMenuKeyChords(menu);

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
        result = displayMenu(menu);
    }
    return result;
}

static int
runSource(Menu* menu, const char* source, const char* filepath)
{
    assert(menu), assert(source), assert(filepath);

    int result = EX_SOFTWARE;

    /* Run preprocessor on `script` and fail if mallformed or other error. */
    char* processedSource = preprocessSource(menu, source, filepath);
    if (!processedSource) return EX_DATAERR;

    /* Begin compilation */
    Compiler compiler = {0};
    result = compileSource(menu, &compiler, processedSource, filepath);
    if (result != EX_OK)
    {
        errorMsg("Could not compile `wks` file: '%s'.", filepath);
        goto fail;
    }

    result = runMenu(menu);

    freeKeyChords(menu->keyChordsHead);
fail:
    free(processedSource);
    return result;
}

/* Read the given '.wks' file, and transpile it into chords.h syntax. */
static int
transpileWksFile(Menu* menu)
{
    assert(menu);

    int result = EX_SOFTWARE;

    /* Read given file to `source` and exit if read failed. */
    char* source = readFile(menu->client.transpile);
    if (!source) return EX_IOERR;

    /* Run the preprocessor on source */
    char* processedSource = preprocessSource(menu, source, menu->client.transpile);
    if (!processedSource)
    {
        result = EX_DATAERR;
        goto end;
    }

    Compiler compiler = {0};
    result = compileSource(menu, &compiler, processedSource, menu->client.transpile);
    if (result != EX_OK) goto fail;

    /* Well formed file, write to stdout. */
    writeBuiltinKeyChordsHeaderFile(menu->keyChordsHead);

    freeKeyChords(menu->keyChordsHead);
fail:
    free(processedSource);
end:
    free(source);
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
    if (!tryStdin(menu)) return EX_IOERR;
    String* source = &menu->client.script;

    result = runSource(menu, source->string, "[SCRIPT]");

    freeString(source);
    return result;
}

/* Read the given '.wks' file, compile it into a Chord array, and execute like normal. */
static int
runWksFile(Menu* menu)
{
    assert(menu);

    int result = EX_SOFTWARE;

    /* Exit on failure to read source file. */
    char* source = readFile(menu->client.wksFile);
    if (!source) return EX_IOERR;

    result = runSource(menu, source, menu->client.wksFile);

    free(source);
    return result;
}

/* Execute precompiled key chords. */
static int
runBuiltinKeyChords(Menu* menu)
{
    assert(menu);

    if (menu->debug) disassembleKeyChords(menu->keyChords, 0);
    return runMenu(menu);
}

int
main(int argc, char** argv)
{
    int result = EX_SOFTWARE;

    Menu menu = {0};
    initMenu(&menu, builtinKeyChords);
    parseArgs(&menu, &argc, &argv);

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

    freeMenuGarbage(&menu);

    return result;
}
