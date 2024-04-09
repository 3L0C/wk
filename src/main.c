#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "config/key_chords.h"

#include "lib/client.h"
#include "lib/common.h"
#include "lib/debug.h"
#include "lib/string.h"
#include "lib/types.h"
#include "lib/util.h"

#include "common.h"
#include "compile.h"
#include "line.h"
#include "preprocessor.h"
#include "writer.h"
#include "transpiler.h"

static Client client;
static WkMenu mainMenu;

static void
freeChords(Chord* chords)
{
    for (size_t i = 0; chords[i].key; i++)
    {
        free(chords[i].key);
        free(chords[i].description);
        free(chords[i].hint);
        free(chords[i].command);
        free(chords[i].before);
        free(chords[i].after);
        if (chords[i].chords) freeChords(chords[i].chords);
    }
    free(chords);
}

/* Read the given '.wks' file, and transpile it into chords.h syntax. */
static int
transpile(void)
{
    int result = EX_OK;
    /* Read given file to `source` and exit if read failed. */
    char* source = readFile(client.transpile);
    if (!source) return EX_IOERR;

    /* Run preprocessor on `source` and fail if file is mallformed or other error. */
    char* processedSource = runPreprocessor(source, client.transpile, client.debug);
    if (!processedSource)
    {
        errorMsg("Failed while running preprocessor on given file.");
        result = EX_DATAERR;
        goto end;
    }
    debugMsg(client.debug, "Contents of preprocessed source: '%s'.", processedSource);

    /* Begin compilation. */
    Compiler compiler;
    initCompiler(&compiler, processedSource);

    /* File is mallformed, fail. */
    if (!transpileChords(&compiler, client.delimiter, client.debug))
    {
        result = EX_DATAERR;
        goto fail;
    }

    /* Well formed file, write to stdout. */
    writeChords(&compiler.lines, client.delimiter);

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

    /* Exit on failure to read stdin. */
    if (!tryStdin(&client)) return EX_IOERR;

    /* Run preprocessor on `script` and fail if mallformed or other error. */
    char* processedSource = runPreprocessor(client.script.string, NULL, client.debug);
    if (!processedSource)
    {
        errorMsg("Failed while running preprocessor on given file.");
        result = EX_DATAERR;
        goto end;
    }
    debugMsg(mainMenu.debug, "Contents of script:\n%s", processedSource);

    /* Begin compilation */
    Compiler compiler;
    initCompiler(&compiler, client.script.string);

    /* User script is mallformed, fail. */
    if (!transpileChords(&compiler, client.delimiter, client.debug))
    {
        errorMsg("Mallformed script: \n%s", client.script);
        result = EX_DATAERR;
        goto fail;
    }

    /* Compile lines, fail if there is nothing to compile. */
    if (!compileChords(&compiler, &mainMenu))
    {
        errorMsg("Could not compile script.");
        result = EX_DATAERR;
        goto fail;
    }

    /* Last bit of prep before running script. */
    countChords(&mainMenu);
    WkStatus status = WK_STATUS_RUNNING;

    /* Pre-press keys */
    if (client.keys)
    {
        status = pressKeys(&mainMenu, client.keys);
    }

    /* If keys were pre-pressed there may be nothing to do, or an error to report. */
    if (status == WK_STATUS_EXIT_SOFTWARE)
    {
        errorMsg("Key(s) not found in key chords: '%s'.", client.keys);
        result = EX_DATAERR;
        goto fail;
    }
    else if (status == WK_STATUS_EXIT_OK)
    {
        debugMsg(mainMenu.debug, "Successfully pressed keys: '%s'.", client.keys);
        result = EX_OK;
    }
    else
    {
        result = displayMenu(&mainMenu);
    }

fail:
    freeChords(mainMenu.chords);
    freeLineArray(&compiler.lines);
    free(processedSource);
end:
    freeString(&client.script);
    return result;
}

/* Read the given '.wks' file, compile it into a Chord array, and execute like normal. */
static int
runChordsFile(void)
{
    int result = EX_SOFTWARE;

    /* Exit on failure to read source file. */
    char* source = readFile(client.chordsFile);
    if (!source) return EX_IOERR;

    /* Run preprocessor on `chordsFile` and fail if mallformed or other error. */
    char* processedSource = runPreprocessor(source, client.chordsFile, client.debug);
    if (!processedSource)
    {
        errorMsg("Failed while running preprocessor on given file.");
        result = EX_DATAERR;
        goto end;
    }
    debugMsg(client.debug, "Contents of preprocessed source: '%s'.", processedSource);

    /* Begin compilation */
    Compiler compiler;
    initCompiler(&compiler, source);

    /* User file is mallformed, fail. */
    if (!transpileChords(&compiler, client.delimiter, client.debug))
    {
        result = EX_DATAERR;
        goto fail;
    }

    /* Compile lines, fail if there is nothing to compile. */
    if (!compileChords(&compiler, &mainMenu))
    {
        result = EX_DATAERR;
        goto fail;
    }

    /* Last bit of prep before running script. */
    countChords(&mainMenu);
    WkStatus status = WK_STATUS_RUNNING;

    /* Pre-press keys */
    if (client.keys)
    {
        status = pressKeys(&mainMenu, client.keys);
    }

    /* If keys were pre-pressed there may be nothing to do, or an error to report. */
    if (status == WK_STATUS_EXIT_SOFTWARE)
    {
        errorMsg("Key(s) not found in key chords: '%s'.", client.keys);
        result = EX_DATAERR;
        goto fail;
    }
    else if (status == WK_STATUS_EXIT_OK)
    {
        debugMsg(mainMenu.debug, "Successfully pressed keys: '%s'.", client.keys);
        result = EX_OK;
    }
    else
    {
        result = displayMenu(&mainMenu);
    }

fail:
    freeChords(mainMenu.chords);
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

    if (mainMenu.debug) debugChords(mainMenu.chords, 0);

    /* Initial setup */
    countChords(&mainMenu);
    WkStatus status = WK_STATUS_RUNNING;

    /* Pre-press keys */
    if (client.keys)
    {
        status = pressKeys(&mainMenu, client.keys);
    }

    /* If keys were pre-pressed there may be nothing to do, or an error to report. */
    if (status == WK_STATUS_EXIT_SOFTWARE)
    {
        errorMsg("Key(s) not found in key chords: '%s'.", client.keys);
        result = EX_DATAERR;
    }
    else if (status == WK_STATUS_EXIT_OK)
    {
        debugMsg(mainMenu.debug, "Successfully pressed keys: '%s'.", client.keys);
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

    initClient(&client, chords);
    parseArgs(&client, &argc, &argv);
    initMenu(&mainMenu, &client);

    if (mainMenu.debug)
    {
        debugMenu(&mainMenu);
        debugClient(&client);
    }

    if (client.transpile)
    {
        result = transpile();
    }
    else if (client.tryScript)
    {
        result = runScript();
    }
    else if (client.chordsFile)
    {
        result = runChordsFile();
    }
    else
    {
        result = runBuiltinKeyChords();
    }

    return result;
}
