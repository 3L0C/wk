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
#include "lib/window.h"

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
    char* source = readFile(client.transpile);
    if (!source) return EX_IOERR;
    char* processedSource = runPreprocessor(source, client.transpile, client.debug);
    if (!processedSource)
    {
        errorMsg("Failed while running preprocessor on given file.");
        result = EX_DATAERR;
        goto end;
    }
    debugMsg(client.debug, "Contents of preprocessed source: '%s'.", processedSource);
    Compiler compiler;
    initCompiler(&compiler, processedSource);
    if (!transpileChords(&compiler, client.delimiter, client.debug))
    {
        result = EX_DATAERR;
        goto fail;
    }
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

    if (!tryStdin(&client)) return EX_IOERR;
    debugMsg(mainMenu.debug, "Contents of script:\n%s", client.script);
    Compiler compiler;
    initCompiler(&compiler, client.script.string);
    if (!transpileChords(&compiler, client.delimiter, client.debug))
    {
        errorMsg("Mallformed script: \n%s", client.script);
        result = EX_DATAERR;
        goto end;
    }
    if (!compileChords(&compiler, &mainMenu))
    {
        errorMsg("Could not compile script.");
        result = EX_DATAERR;
        goto error;
    }
    countChords(&mainMenu);
    if (client.keys)
    {
        WkStatus status = pressKeys(&mainMenu, client.keys);
        if (status == WK_STATUS_EXIT_SOFTWARE)
        {
            result = EX_DATAERR;
            goto error;
        }
        else if (status == WK_STATUS_EXIT_OK)
        {
            debugMsg(mainMenu.debug, "Successfull pressed keys: '%s'.", client.keys);
            result = EX_OK;
        }
        else
        {
            result = displayMenu(&mainMenu);
        }
    }
    else
    {
        result = displayMenu(&mainMenu);
    }

error:
    freeChords(mainMenu.chords);
end:
    freeLineArray(&compiler.lines);
    freeString(&client.script);
    return result;
}

/* Read the given '.wks' file, compile it into a Chord array, and execute like normal. */
static int
runChordsFile(void)
{
    int result = EX_SOFTWARE;
    char* source = readFile(client.chordsFile);
    if (!source) return EX_IOERR;
    Compiler compiler;
    initCompiler(&compiler, source);
    if (!transpileChords(&compiler, client.delimiter, client.debug))
    {
        result = EX_DATAERR;
        goto end;
    }
    if (!compileChords(&compiler, &mainMenu))
    {
        result = EX_DATAERR;
        goto error;
    }
    countChords(&mainMenu);
    if (client.keys)
    {
        WkStatus status = pressKeys(&mainMenu, client.keys);
        if (status == WK_STATUS_EXIT_SOFTWARE)
        {
            result = EX_DATAERR;
            goto error;
        }
        else if (status == WK_STATUS_EXIT_OK)
        {
            debugMsg(mainMenu.debug, "Successfull pressed keys: '%s'.", client.keys);
            result = EX_OK;
        }
        else
        {
            result = displayMenu(&mainMenu);
        }
    }
    else
    {
        result = displayMenu(&mainMenu);
    }

error:
    freeChords(mainMenu.chords);
end:
    freeLineArray(&compiler.lines);
    free(source);
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
        if (mainMenu.debug)
        {
            debugChords(mainMenu.chords, 0);
        }

        countChords(&mainMenu);

        if (client.keys)
        {
            WkStatus status = pressKeys(&mainMenu, client.keys);
            if (statusIsError(status))
            {
                errorMsg("Key(s) not found in chords: '%s'.", client.keys);
                return EX_DATAERR;
            }
            else if (status == WK_STATUS_EXIT_OK)
            {
                return EX_OK;
            }
        }

        result = displayMenu(&mainMenu);
    }

    return result;
}
