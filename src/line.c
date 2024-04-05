#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/memory.h"
#include "lib/types.h"

#include "line.h"
#include "token.h"

static void
copyLineArray(LineArray* from, LineArray* to)
{
    if (from->count == 0)
    {
        initLineArray(to);
        return;
    }
    to->lines = ALLOCATE(Line, from->capacity);
    to->capacity = from->capacity;
    to->count = from->count;
    for (size_t i = 0; i < from->count; i++)
    {
        copyLine(&from->lines[i], &to->lines[i]);
    }
}

void
copyLine(Line* from, Line* to)
{
    assert(from && to);

    to->index       = from->index;
    to->mods        = from->mods;
    to->key         = from->key;
    copyTokenArray(&from->description, &to->description);
    copyTokenArray(&from->command, &to->command);
    copyTokenArray(&from->before, &to->before);
    copyTokenArray(&from->after, &to->after);
    COPY_FLAGS(from->flags, to->flags);
    copyLineArray(&from->array, &to->array);
}

void
copyMissing(Line* from, Line* to)
{
    assert(from && to);

    if (to->description.count == 0) copyTokenArray(&from->description, &to->description);
    if (to->command.count == 0) copyTokenArray(&from->command, &to->command);
    if (to->before.count == 0) copyTokenArray(&from->before, &to->before);
    if (to->after.count == 0) copyTokenArray(&from->after, &to->after);
    if (!HAS_FLAG(to->flags)) COPY_FLAGS(from->flags, to->flags);
    copyLineArray(&from->array, &to->array);
}

void
freeLine(Line* line)
{
    assert(line);

    freeTokenArray(&line->description);
    freeTokenArray(&line->command);
    freeTokenArray(&line->before);
    freeTokenArray(&line->after);
    freeLineArray(&line->array);
    initLine(line);
}

void
freeLineArray(LineArray* array)
{
    assert(array);

    for (size_t i = 0; i < array->count; i++)
    {
        freeLine(&array->lines[i]);
    }
    FREE_ARRAY(Line, array->lines, array->capacity);
    initLineArray(array);
}

void
initLine(Line* line)
{
    assert(line);

    line->index         = -1;
    RESET_MODS(line->mods);
    initTokenArray(&line->description);
    initTokenArray(&line->command);
    initTokenArray(&line->before);
    initTokenArray(&line->after);
    RESET_FLAGS(line->flags);
    initLineArray(&line->array);
}

void
initLineArray(LineArray* array)
{
    assert(array);

    array->lines    = NULL;
    array->capacity = 0;
    array->count    = 0;
}

void
writeLineArray(LineArray* array, Line* line)
{
    assert(array && line);

    if (array->capacity < array->count + 1)
    {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->lines = GROW_ARRAY(
            Line, array->lines, oldCapacity, array->capacity
        );
    }

    copyLine(line, &array->lines[array->count]);
    array->count++;
}
