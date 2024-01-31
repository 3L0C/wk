#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "lib/client.h"
#include "lib/common.h"
#include "lib/debug.h"
#include "lib/memory.h"
#include "lib/types.h"
#include "lib/util.h"
#include "lib/window.h"

#include "chords.h"
#include "common.h"
#include "compile.h"
#include "line.h"
#include "writer.h"
#include "transpiler.h"

static Client client;
static WkProperties properties;

static void
freeChords(Chord* chords)
{
    while (chords->key)
    {
        free(chords->key);
        free(chords->description);
        free(chords->hint);
        free(chords->command);
        free(chords->before);
        free(chords->after);
        if (chords->chords) freeChords(chords->chords);
    }
    free(chords);
}

/* Read the given '.wks' file, and transpile it into chords.h syntax. */
static int
transpile(void)
{
    int result = EX_SOFTWARE;
    char* source = readFile(client.transpile);
    if (!source) return EX_IOERR;
    Compiler compiler;
    initCompiler(&compiler, source);
    if (!transpileChords(&compiler, client.delimiter, client.debug))
    {
        result = EX_DATAERR;
        goto end;
    }
    writeChords(&compiler.lines, client.delimiter);

end:
    freeLineArray(&compiler.lines);
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
    Compiler compiler;
    initCompiler(&compiler, client.script);
    if (!transpileChords(&compiler, client.delimiter, client.debug))
    {
        result = EX_DATAERR;
        goto end;
    }
    if (!compileChords(&compiler, &properties))
    {
        free(client.script);
        return EX_DATAERR;
    }
    if (client.keys) pressKeys(&properties, client.keys);
    result = run(&properties);

end:
    freeChords(properties.chords);
    freeLineArray(&compiler.lines);
    free(client.script);
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
    if (!compileChords(&compiler, &properties))
    {
        result = EX_DATAERR;
        goto end;
    }
    if (client.keys) pressKeys(&properties, client.keys);
    result = run(&properties);

end:
    freeChords(properties.chords);
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
    initProperties(&properties, &client);

    if (properties.debug)
    {
        debugProperties(&properties);
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
        if (properties.debug)
        {
            debugChords(properties.chords, 0);
        }
        result = run(&properties);
    }

    return result;
}
