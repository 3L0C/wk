#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* common includes */
#include "common/debug.h"
#include "common/memory.h"

/* local includes */
#include "token.h"

void
copyToken(Token* from, Token* to)
{
    to->type = from->type;
    to->start = from->start;
    to->length = from->length;
    to->line = from->line;
    to->column = from->column;
    to->message = from->message;
    to->messageLength = from->messageLength;
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
errorAtToken(Token* token, const char* filepath, const char* fmt, ...)
{
    assert(token && filepath && fmt);

    fprintf(stderr, "%s:%u:%u: error", filepath, token->line, token->column);

    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end: ");
    }
    else if (token->type == TOKEN_ERROR)
    {
        fprintf(
            stderr,
            " at line %u: '%.*s'\n\t",
            token->line, (int)token->length, token->start
        );
    }
    else
    {
        fprintf(stderr, " at '%.*s': ", (int)token->length, token->start);
    }

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr);
}

bool
getDoubleFromToken(Token* token, double* dest, bool debug)
{
    char* endptr;
    int oldErrno = errno;
    errno = 0;
    double value = strtod(token->start, &endptr);
    *dest = 0;

    if (errno == ERANGE)
    {
        debugMsg(
            debug,
            "Tried to parse string as double, but it was out of range: '%.*s'",
            token->length, token->start
        );
        errno = oldErrno;
        return false;
    }
    else if ((size_t)(endptr - token->start) != token->length)
    {
        debugMsg(
            debug,
            "Tried to parse string as double, but it was parsed differently than expected.\n\t"
            "String to be parsed:    '%.*s'\n\t"
            "String actually parsed: '%.*s'",
            token->length, token->start,
            (int)(endptr - token->start), token->start
        );
        return false;
    }

    *dest = (int32_t)value;
    return true;
}

bool
getInt32FromToken(Token* token, int32_t* dest, bool debug)
{
    char* endptr;
    int oldErrno = errno;
    errno = 0;
    intmax_t value = strtoimax(token->start, &endptr, 10);
    *dest = 0;

    if (errno == ERANGE)
    {
        debugMsg(
            debug,
            "Tried to parse string as uintmax_t, but it was out of range: '%.*s'",
            token->length, token->start
        );
        errno = oldErrno;
        return false;
    }
    else if (value > INT32_MAX)
    {
        debugMsg(
            debug,
            "Tried to parse string as int32_t, but it was too large: '%.*s'",
            token->length, token->start
        );
        return false;
    }
    else if ((size_t)(endptr - token->start) != token->length)
    {
        debugMsg(
            debug,
            "Tried to parse string as int32_t, but it was parsed differently than expected.\n\t"
            "String to be parsed:    '%.*s'\n\t"
            "String actually parsed: '%.*s'",
            token->length, token->start,
            (int)(endptr - token->start), token->start
        );
        return false;
    }

    *dest = (int32_t)value;
    return true;
}

bool
getUint32FromToken(Token* token, uint32_t* dest, bool debug)
{
    char* endptr;
    int oldErrno = errno;
    errno = 0;
    uintmax_t value = strtoumax(token->start, &endptr, 10);
    *dest = 0;

    if (errno == ERANGE)
    {
        debugMsg(
            debug,
            "Tried to parse string as uintmax_t, but it was out of range: '%.*s'",
            token->length, token->start
        );
        errno = oldErrno;
        return false;
    }
    else if (value > UINT32_MAX)
    {
        debugMsg(
            debug,
            "Tried to parse string as uint32_t, but it was too large: '%.*s'",
            token->length, token->start
        );
        return false;
    }
    else if ((size_t)(endptr - token->start) != token->length)
    {
        debugMsg(
            debug,
            "Tried to parse string as uint32_t, but it was parsed differently than expected.\n\t"
            "String to be parsed:    '%.*s'\n\t"
            "String actually parsed: '%.*s'",
            token->length, token->start,
            (int)(endptr - token->start), token->start
        );
        return false;
    }

    *dest = (uint32_t)value;
    return true;
}

void
initToken(Token* token)
{
    token->type = TOKEN_EMPTY;
    token->start = NULL;
    token->length = 0;
    token->line = 0;
    token->column = 0;
    token->message = NULL;
    token->messageLength = 0;
}

void
initTokenArray(TokenArray* array)
{
    assert(array);

    array->tokens = NULL;
    array->count = 0;
    array->capacity = 0;
}

void
freeTokenArray(TokenArray* array)
{
    assert(array);

    FREE_ARRAY(Token, array->tokens, array->capacity);
    array->count = 0;
    array->capacity = 0;
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
