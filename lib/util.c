#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

bool
resizeBuffer(char** ioBuffer, size_t* ioSize, size_t nsize)
{
    assert(ioBuffer && ioSize);

    if (nsize == 0 || nsize <= *ioSize) return false;

    void* tmp = realloc(*ioBuffer, nsize);
    if (!tmp) return false;

    *ioBuffer = tmp;
    *ioSize = nsize;

    return true;
}

bool
vrprintf(char** ioBuffer, size_t* ioLen, const char* fmt, va_list args)
{
    assert(ioBuffer && ioLen && fmt);

    va_list copy;
    va_copy(copy, args);

    size_t len = vsnprintf(NULL, 0, fmt, args) + 1;

    if ((!*ioBuffer || *ioLen < len) && !resizeBuffer(ioBuffer, ioLen, len))
        return false;

    vsnprintf(*ioBuffer, len, fmt, copy);
    va_end(copy);

    return true;
}

uint32_t
countChords(const Chord* chords)
{
    assert(chords);

    uint32_t result = 0;
    while (chords[result].key) result++;
    return result;
}

void
calculateGrid(const uint32_t count, const uint32_t maxCols, uint32_t* rows, uint32_t* cols)
{
    assert(rows && cols);

    if (maxCols == 0 || maxCols >= count)
    {
        *rows = 1;
        *cols = count;
    }
    else
    {
        *rows = (count + maxCols - 1) / maxCols;
        *cols = (count + *rows - 1) / *rows;
    }
}
