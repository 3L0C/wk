#include <assert.h>
#include <string.h>

#include "lib/memory.h"

#include "token.h"
#include "debug.h"
#include "scanner.h"

static void
copyToken(Token* from, Token* to)
{
    to->type            = from->type;
    to->start           = from->start;
    to->length          = from->length;
    to->line            = from->line;
    to->message         = from->message;
    to->messageLength   = from->messageLength;
}

void
copyTokenArray(TokenArray* from, TokenArray* to)
{
    assert(from && to);

    if (from->count == 0)
    {
        initTokenArray(to);
        return;
    }
    to->tokens = ALLOCATE(Token, from->capacity);
    to->count = from->count;
    to->capacity = from->capacity;
    for (size_t i = 0; i < to->count; i++)
    {
        copyToken(&from->tokens[i], &to->tokens[i]);
    }
}

void
initTokenArray(TokenArray* array)
{
    assert(array);

    array->tokens = NULL;
    array->capacity = 0;
    array->count = 0;
}

void
freeTokenArray(TokenArray* array)
{
    assert(array);

    FREE_ARRAY(Token, array->tokens, array->capacity);
}

void
writeTokenArray(TokenArray* array, Token* token)
{
    assert(array && token);

    if (array->capacity < array->count + 1)
    {
        size_t oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->tokens = GROW_ARRAY(
            Token, array->tokens, oldCapacity, array->capacity
        );
    }

    copyToken(token, &array->tokens[array->count]);
    array->count++;
}
