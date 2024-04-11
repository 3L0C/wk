#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "config/key_chords.h"

/* common includes */
#include "common/common.h"
#include "common/debug.h"
#include "common/string.h"
#include "common/types.h"
#include "common/util.h"

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

/* Read the given '.wks' file, and transpile it into chords.h syntax. */
static int
transpile(void)
{
    int result = EX_OK;
    /* Read given file to `source` and exit if read failed. */
    char* source = readFile(mainMenu.client.transpile);
    if (!source) return EX_IOERR;

    /* Run preprocessor on `source` and fail if file is mallformed or other error. */
    char* processedSource = runPreprocessor(&mainMenu, source, mainMenu.client.transpile);
    if (!processedSource)
    {
        errorMsg("Failed while running preprocessor on given file.");
        result = EX_DATAERR;
        goto end;
    }
    debugMsg(mainMenu.debug, "Contents of preprocessed source: '%s'.", processedSource);

    /* Begin compilation. */
    Compiler compiler;
    initCompiler(&compiler, processedSource);

    /* File is mallformed, fail. */
    if (!transpileChords(&compiler, mainMenu.delimiter, mainMenu.debug))
    {
        errorMsg("Mallformed `wks` file. Use `man 5 wks` to learn about proper syntax.");
        result = EX_DATAERR;
        goto fail;
    }

    /* Well formed file, write to stdout. */
    writeChords(&compiler.lines, mainMenu.delimiter);

fail:
    freeLineArray(&compiler.lines);
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
    mainMenu.keyChords = NULL; /* Prevent invalid free on failure. */

    /* Exit on failure to read stdin. */
    if (!tryStdin(&mainMenu)) return EX_IOERR;

    /* Run preprocessor on `script` and fail if mallformed or other error. */
    char* processedSource = runPreprocessor(&mainMenu, mainMenu.client.script.string, NULL);
    if (!processedSource)
    {
        errorMsg("Failed while running preprocessor on given file.");
        result = EX_DATAERR;
        goto end;
    }
    debugMsg(mainMenu.debug, "Contents of script:\n%s", processedSource);

    /* Begin compilation */
    Compiler compiler;
    initCompiler(&compiler, processedSource);

    /* User script is mallformed, fail. */
    if (!transpileChords(&compiler, mainMenu.delimiter, mainMenu.debug))
    {
        errorMsg("Mallformed script: \n%s", mainMenu.client.script);
        result = EX_DATAERR;
        goto fail;
    }

    /* Compile lines, fail if there is nothing to compile. */
    if (!compileKeyChords(&compiler, &mainMenu))
    {
        errorMsg("Could not compile script.");
        result = EX_DATAERR;
        goto fail;
    }

    /* Last bit of prep before running script. */
    countChords(&mainMenu);
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
        goto fail;
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

fail:
    freeKeyChords(mainMenu.keyChords);
    freeLineArray(&compiler.lines);
    free(processedSource);
end:
    freeString(&mainMenu.client.script);
    return result;
}

/* Read the given '.wks' file, compile it into a Chord array, and execute like normal. */
static int
runChordsFile(void)
{
    int result = EX_SOFTWARE;
    mainMenu.keyChords = NULL; /* Prevent invalid free on failure. */

    /* Exit on failure to read source file. */
    char* source = readFile(mainMenu.client.keyChordsFile);
    if (!source) return EX_IOERR;

    /* Run preprocessor on `chordsFile` and fail if mallformed or other error. */
    char* processedSource = runPreprocessor(&mainMenu, source, mainMenu.client.keyChordsFile);
    if (!processedSource)
    {
        errorMsg("Failed while running preprocessor on given file.");
        result = EX_DATAERR;
        goto end;
    }
    debugMsg(mainMenu.debug, "Contents of preprocessed source: '%s'.", processedSource);

    /* Begin compilation */
    Compiler compiler;
    initCompiler(&compiler, processedSource);

    /* User file is mallformed, fail. */
    if (!transpileChords(&compiler, mainMenu.delimiter, mainMenu.debug))
    {
        result = EX_DATAERR;
        goto fail;
    }

    /* Compile lines, fail if there is nothing to compile. */
    if (!compileKeyChords(&compiler, &mainMenu))
    {
        result = EX_DATAERR;
        goto fail;
    }

    /* Last bit of prep before running script. */
    countChords(&mainMenu);
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
        goto fail;
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
    countChords(&mainMenu);
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
