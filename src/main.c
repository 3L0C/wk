#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "lib/client.h"
#include "lib/common.h"
#include "lib/memory.h"
#include "lib/window.h"

#include "chords.h"
#include "common.h"
#include "compile.h"
#include "transpiler.h"

static Client client;
static WkProperties props;

static int
parseFile(void)
{
    int result = EX_SOFTWARE;
    char* source = readFile(client.parse);
    if (!source) return EX_IOERR;
    result = transpileChords(source, client.delimiter);
    free(source);
    return result;
}

static int
runScript(void)
{
    if (!tryStdin(&client)) return EX_IOERR;
    if (!compileChords(&props, client.script))
    {
        free(client.script);
        return EX_DATAERR;
    }
    pressKeys(&props, client.keys);
    return run(&props);
}

static int
runChordsFile(void)
{
    char* source = readFile(client.chordsFile);
    if (!source) return EX_IOERR;
    if (!compileChords(&props, source))
    {
        free(source);
        return EX_DATAERR;
    }
    pressKeys(&props, client.keys);
    return run(&props);
}

int
main(int argc, char** argv)
{
    int result = EX_SOFTWARE;

    initClient(&client, chords);
    parseArgs(&client, &argc, &argv);
    initProperties(&props, &client);

    if (client.parse)
    {
        result = parseFile();
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
        result = run(&props);
    }

    return result;
}
