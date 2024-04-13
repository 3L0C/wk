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
#include "common/types.h"

/* compiler includes */
#include "compiler/common.h"
#include "compiler/compile.h"
#include "compiler/line.h"
#include "compiler/preprocessor.h"
#include "compiler/writer.h"
#include "compiler/transpiler.h"

static WkMenu mainMenu;

static void
freeKeyChords(WkKeyChord* keyChords)
{
    if (!keyChords) return;

    for (size_t i = 0; keyChords[i].key; i++)
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

static char*
preprocessSources(char* source, const char* filepath)
{
    assert(source);

    char* processedSource = runPreprocessor(&mainMenu, source, filepath);
    if (!processedSource)
    {
        errorMsg("Failed while running preprocessor on given file.");
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
transpileSource(Compiler* compiler, char* source, const char* filepath)
{
    assert(compiler);

    int result = EX_OK;

    /* Run preprocessor on `source` and fail if file is mallformed or other error. */
    char* processedSource = runPreprocessor(&mainMenu, source, filepath);
    if (!processedSource)
    {
        errorMsg("Failed while running preprocessor on given file.");
        result = EX_DATAERR;
        goto fail;
    }
    if (mainMenu.debug)
    {
        debugPrintHeader(" Contents of Preprocessed Source ");
        debugMsg(true, "|");
        debugTextWithLineNumber(processedSource);
        debugMsg(true, "|");
        debugPrintHeader("");
    }

    /* Begin compilation */
    initCompiler(compiler, processedSource, filepath);

    /* File is mallformed, fail. */
    if (!transpileChords(compiler, mainMenu.delimiter, mainMenu.debug))
    {
        errorMsg("Mallformed `wks` file. Use `man 5 wks` to learn about the syntax.");
        result = EX_DATAERR;
        goto fail;
    }

fail:
    return result;
}

static int
compileKeyChordsToMenu(Compiler* compiler)
{
    assert(compiler);

    /* Compile lines, fail if there is nothing to compile. */
    if (!compileKeyChords(compiler, &mainMenu)) return EX_DATAERR;
    return EX_OK;
}

static int
runMenu(Compiler* compiler)
{
    assert(compiler);

    int result = EX_SOFTWARE;
    WkStatus status = WK_STATUS_RUNNING;
    countMenuKeyChords(&mainMenu);

    /* Pre-press keys */
    if (mainMenu.client.keys) status = pressKeys(&mainMenu, mainMenu.client.keys);

    /* If keys were pre-pressed there may be nothing to do, or an error to report. */
    if (status == WK_STATUS_EXIT_SOFTWARE)
    {
        errorMsg("Key(s) not found in key chords: '%s'.", mainMenu.client.keys);
        result = EX_DATAERR;
    }
    else if (status == WK_STATUS_EXIT_OK)
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

/* Read the given '.wks' file, and transpile it into chords.h syntax. */
static int
transpile(void)
{
    int result = EX_SOFTWARE;
    /* Read given file to `source` and exit if read failed. */
    char* source = readFile(mainMenu.client.transpile);
    if (!source) return EX_IOERR;

    /* Run the preprocessor on source */
    char* processedSource = preprocessSources(source, mainMenu.client.transpile);
    if (!processedSource)
    {
        result = EX_DATAERR;
        goto end;
    }

    Compiler compiler;
    result = transpileSource(&compiler, source, mainMenu.client.transpile);
    if (result != EX_OK) goto fail;

    /* Well formed file, write to stdout. */
    writeChords(&compiler.lines, mainMenu.delimiter);

fail:
    freeLineArray(&compiler.lines);
    free(compiler.source);
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

    /* Prevent invalid free on failure. */
    mainMenu.keyChords = NULL;

    /* Exit on failure to read stdin. */
    if (!tryStdin(&mainMenu)) return EX_IOERR;
    String* source = &mainMenu.client.script;

    /* Run preprocessor on `script` and fail if mallformed or other error. */
    char* processedSource = preprocessSources(source->string, NULL);
    if (!processedSource)
    {
        result = EX_DATAERR;
        goto end;
    }

    /* Begin compilation */
    Compiler compiler;
    result = transpileSource(&compiler, source->string, mainMenu.client.transpile);
    if (result != EX_OK) goto fail;

    result = compileKeyChordsToMenu(&compiler);
    if (result != EX_OK)
    {
        errorMsg("Could not compile script.");
        goto fail;
    }

    result = runMenu(&compiler);

fail:
    freeKeyChords(mainMenu.keyChords);
    freeLineArray(&compiler.lines);
    free(processedSource);
end:
    freeString(source);
    return result;
}

/* Read the given '.wks' file, compile it into a Chord array, and execute like normal. */
static int
runChordsFile(void)
{
    int result = EX_SOFTWARE;

    /* Prevent invalid free on failure. */
    mainMenu.keyChords = NULL;

    /* Exit on failure to read source file. */
    char* source = readFile(mainMenu.client.keyChordsFile);
    if (!source) return EX_IOERR;

    /* Run preprocessor on `chordsFile` and fail if mallformed or other error. */
    char* processedSource = preprocessSources(source, mainMenu.client.keyChordsFile);
    if (!processedSource)
    {
        result = EX_DATAERR;
        goto end;
    }

    /* Begin compilation */
    Compiler compiler;
    result = transpileSource(&compiler, source, mainMenu.client.keyChordsFile);
    if (result != EX_OK) goto fail;

    result = compileKeyChordsToMenu(&compiler);
    if (result != EX_OK)
    {
        errorMsg("Could not compile script.");
        goto fail;
    }

    result = runMenu(&compiler);

fail:
    freeKeyChords(mainMenu.keyChords);
    freeLineArray(&compiler.lines);
    free(processedSource);
end:
    free(source);
    return result;
}

/* Execute precompiled key chords. */
static int
runBuiltinKeyChords()
{
    int result = EX_SOFTWARE;

    if (mainMenu.debug) debugKeyChords(mainMenu.keyChords, 0);

    /* Initial setup */
    countMenuKeyChords(&mainMenu);
    WkStatus status = WK_STATUS_RUNNING;

    /* Pre-press keys */
    if (mainMenu.client.keys)
    {
        status = pressKeys(&mainMenu, mainMenu.client.keys);
    }

    /* If keys were pre-pressed there may be nothing to do, or an error to report. */
    if (status == WK_STATUS_EXIT_SOFTWARE)
    {
        errorMsg("Key(s) not found in key chords: '%s'.", mainMenu.client.keys);
        result = EX_DATAERR;
    }
    else if (status == WK_STATUS_EXIT_OK)
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

int
main(int argc, char** argv)
{
    int result = EX_SOFTWARE;

    initMenu(&mainMenu, keyChords);
    parseArgs(&mainMenu, &argc, &argv);

    if (mainMenu.debug) debugMenu(&mainMenu);

    if (mainMenu.client.transpile)
    {
        result = transpile();
    }
    else if (mainMenu.client.tryScript)
    {
        result = runScript();
    }
    else if (mainMenu.client.keyChordsFile)
    {
        result = runChordsFile();
    }
    else
    {
        result = runBuiltinKeyChords();
    }

    return result;
}
