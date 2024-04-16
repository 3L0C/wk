#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

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

static Menu mainMenu;

static void
freeKeyChords(KeyChord* keyChords)
{
    if (!keyChords) return;

    for (size_t i = 0; keyChords[i].state == KEY_CHORD_STATE_NOT_NULL; i++)
    {
        free(keyChords[i].key);
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
freeMenuGarbage(void)
{
    if (mainMenu.garbage.shell) free(mainMenu.garbage.shell);
    if (mainMenu.garbage.font) free(mainMenu.garbage.font);
    if (mainMenu.garbage.foregroundColor) free(mainMenu.garbage.foregroundColor);
    if (mainMenu.garbage.backgroundColor) free(mainMenu.garbage.backgroundColor);
    if (mainMenu.garbage.borderColor) free(mainMenu.garbage.borderColor);
}

static char*
preprocessSource(const char* source, const char* filepath)
{
    assert(source);

    char* processedSource = runPreprocessor(&mainMenu, source, filepath);
    if (!processedSource)
    {
        errorMsg("Failed while running preprocessor on `wks` file: '%s'.", filepath);
    }
    else if (mainMenu.debug)
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
compileSource(Compiler* compiler, char* source, const char* filepath)
{
    assert(compiler && source && filepath);

    initCompiler(compiler, source, filepath);

    /* Compile lines, retruns null on error. */
    mainMenu.keyChordsHead = mainMenu.keyChords = compileKeyChords(compiler, &mainMenu);
    if (!mainMenu.keyChords) return EX_DATAERR;

    return EX_OK;
}

static int
runMenu(void)
{
    int result = EX_SOFTWARE;
    MenuStatus status = MENU_STATUS_RUNNING;
    countMenuKeyChords(&mainMenu);

    /* Pre-press keys */
    if (mainMenu.client.keys)
    {
        if (mainMenu.debug) debugMsg(true, "Trying to press key(s): '%s'.", mainMenu.client.keys);
        status = pressKeys(&mainMenu, mainMenu.client.keys);
    }

    /* If keys were pre-pressed there may be nothing to do, or an error to report. */
    if (status == MENU_STATUS_EXIT_SOFTWARE)
    {
        errorMsg("Key(s) not found in key chords: '%s'.", mainMenu.client.keys);
        result = EX_DATAERR;
    }
    else if (status == MENU_STATUS_EXIT_OK)
    {
        debugMsg(mainMenu.debug, "Successfully pressed keys: '%s'.", mainMenu.client.keys);
        result = EX_OK;
    }
    else
    {
        result = displayMenu(&mainMenu);
    }
    return result;
}

static int
runSource(const char* source, const char* filepath)
{
    int result = EX_SOFTWARE;

    /* Run preprocessor on `script` and fail if mallformed or other error. */
    char* processedSource = preprocessSource(source, filepath);
    if (!processedSource) return EX_DATAERR;

    /* Begin compilation */
    Compiler compiler = {0};
    result = compileSource(&compiler, processedSource, filepath);
    if (result != EX_OK)
    {
        errorMsg("Could not compile `wks` file: '%s'.", filepath);
        goto fail;
    }

    result = runMenu();

fail:
    freeKeyChords(mainMenu.keyChordsHead);
    free(processedSource);
    return result;
}

/* Read the given '.wks' file, and transpile it into chords.h syntax. */
static int
transpileWksFile(void)
{
    int result = EX_SOFTWARE;

    /* Read given file to `source` and exit if read failed. */
    char* source = readFile(mainMenu.client.transpile);
    if (!source) return EX_IOERR;

    /* Run the preprocessor on source */
    char* processedSource = preprocessSource(source, mainMenu.client.transpile);
    if (!processedSource)
    {
        result = EX_DATAERR;
        goto end;
    }

    Compiler compiler;
    result = compileSource(&compiler, processedSource, mainMenu.client.transpile);
    if (result != EX_OK) goto fail;

    /* Well formed file, write to stdout. */
    writeBuiltinKeyChordsHeaderFile(mainMenu.keyChordsHead);

    freeKeyChords(mainMenu.keyChordsHead);
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
runScript(void)
{
    int result = EX_SOFTWARE;

    /* Exit on failure to read stdin. */
    if (!tryStdin(&mainMenu)) return EX_IOERR;
    String* source = &mainMenu.client.script;

    result = runSource(source->string, "[SCRIPT]");

    freeString(source);
    return result;
}

/* Read the given '.wks' file, compile it into a Chord array, and execute like normal. */
static int
runWksFile(void)
{
    int result = EX_SOFTWARE;

    /* Exit on failure to read source file. */
    char* source = readFile(mainMenu.client.wksFile);
    if (!source) return EX_IOERR;

    result = runSource(source, mainMenu.client.wksFile);

    free(source);
    return result;
}

/* Execute precompiled key chords. */
static int
runBuiltinKeyChords()
{
    if (mainMenu.debug) disassembleKeyChords(mainMenu.keyChords, 0);
    return runMenu();
}

int
main(int argc, char** argv)
{
    int result = EX_SOFTWARE;

    initMenu(&mainMenu, builtinKeyChords);
    parseArgs(&mainMenu, &argc, &argv);

    if (mainMenu.debug) disassembleMenu(&mainMenu);

    if (mainMenu.client.transpile)
    {
        result = transpileWksFile();
    }
    else if (mainMenu.client.tryScript)
    {
        result = runScript();
    }
    else if (mainMenu.client.wksFile)
    {
        result = runWksFile();
    }
    else
    {
        result = runBuiltinKeyChords();
    }

    freeMenuGarbage();

    return result;
}
