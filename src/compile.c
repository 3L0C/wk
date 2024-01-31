#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "lib/common.h"
#include "lib/memory.h"
#include "lib/types.h"

#include "compile.h"
#include "line.h"

static const char* delimiter;

bool
compileChords(Compiler* compiler, WkProperties* props)
{
    assert(compiler && props);

    if (compiler->lines.count == 0)
    {
        warnMsg("Nothing to compile.");
        return false;
    }

    delimiter = props->delimiter;

    LineArray* lines = &compiler->lines;
    size_t count = lines->count;
    Chord* chords = ALLOCATE(Chord, count);

    props->chords = chords;
    return true;
}

void
initCompiler(Compiler* compiler, const char* source)
{
    initScanner(&compiler->scanner, source);
    compiler->hadError      = false;
    compiler->panicMode     = false;
    compiler->index         = 0;
    initLine(&compiler->line);
    compiler->lineDest      = &compiler->lines;
    compiler->linePrefix    = NULL;
    initLineArray(&compiler->lines);
}
