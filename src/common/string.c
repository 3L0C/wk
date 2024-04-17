#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "string.h"
#include "memory.h"

typedef int (*ConversionFp)(int);

void
appendInt32ToString(String* dest, int32_t i)
{
    size_t len = snprintf(NULL, 0, "%d", i);
    char buffer[len + 1]; /* +1 for null byte. */
    snprintf(buffer, len + 1, "%d", i);
    appendToString(dest, buffer, len);
}

void
appendUInt32ToString(String* dest, uint32_t i)
{
    size_t len = snprintf(NULL, 0, "%u", i);
    char buffer[len + 1]; /* +1 for null byte. */
    snprintf(buffer, len + 1, "%u", i);
    appendToString(dest, buffer, len);
}

void
appendCharToString(String* dest, char c)
{
    assert(dest);

    char buffer[] = { c, '\0' };
    appendToString(dest, buffer, 1);
}

void
appendToString(String *dest, const char* source, size_t len)
{
    /* assert(dest && source); */
    assert(dest);
    assert(source);
    if (len < 1) return;

    while (dest->count + len + 1 > dest->capacity)
    {
        size_t oldCapacity = dest->capacity;
        dest->capacity = GROW_CAPACITY(oldCapacity);
        dest->string = GROW_ARRAY(
            char, dest->string, oldCapacity, dest->capacity
        );
    }

    memcpy(dest->string + dest->count, source, len);
    dest->count += len;
    dest->string[dest->count] = '\0';
}

static void
appendCharsToStringWithFunc(String* dest, const char* source, size_t len, ConversionFp fp)
{
    assert(dest && source);
    if (len < 1) return;

    for (size_t i = 0; i < len; i++)
    {
        appendCharToString(dest, fp(source[i]));
    }
}

void
appendToStringWithState(String* dest, const char* source, size_t len, StringAppendState state)
{
    assert(dest && source);
    if (len < 1) return;

    switch (state)
    {
    case STRING_APPEND_UPPER_FIRST:
    {
        appendCharToString(dest, toupper(*source));
        appendToString(dest, source + 1, len - 1);
        break;
    }
    case STRING_APPEND_LOWER_FIRST:
    {
        appendCharToString(dest, tolower(*source));
        appendToString(dest, source + 1, len - 1);
        break;
    }
    case STRING_APPEND_UPPER_ALL:
    {
        appendCharToString(dest, toupper(*source));
        appendCharsToStringWithFunc(dest, source + 1, len - 1, toupper);
        break;
    }
    case STRING_APPEND_LOWER_ALL:
    {
        appendCharToString(dest, tolower(*source));
        appendCharsToStringWithFunc(dest, source + 1, len - 1, tolower);
        break;
    }
    default: break;
    }
}

void
disownString(String* string)
{
    string->string = NULL;
    string->count = 0;
    string->capacity = 0;
}

void
freeString(String* string)
{
    FREE_ARRAY(char, string->string, string->count);
    string->string = NULL;
    string->count = 0;
    string->capacity = 0;
}

void
initFromCharString(String* dest, char* source)
{
    size_t len = strlen(source);
    dest->string = source;
    dest->count = len;
    dest->capacity = len;
}

void
initString(String *string)
{
    string->string = NULL;
    string->count = 0;
    string->capacity = 0;
}

void
rtrimString(String* string)
{
    assert(string);
    if (string->count == 0) return;

    while (string->count > 0 && isspace(string->string[string->count - 1]))
    {
        string->count--;
    }

    string->string[string->count] = '\0';
}
