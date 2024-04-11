#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "string.h"
#include "memory.h"

void
appendInt32ToString(String* string, int32_t i)
{
    size_t len = snprintf(NULL, 0, "%d", i);
    char buffer[len + 1]; /* +1 for null byte. */
    snprintf(buffer, len + 1, "%d", i);
    appendToString(string, buffer, len);
}

void
appendUInt32ToString(String* string, uint32_t i)
{
    size_t len = snprintf(NULL, 0, "%u", i);
    char buffer[len + 1]; /* +1 for null byte. */
    snprintf(buffer, len + 1, "%u", i);
    appendToString(string, buffer, len);
}

void
appendCharToString(String* string, char c)
{
    assert(string);

    char buffer[] = { c, '\0' };
    appendToString(string, buffer, 1);
}

void
appendToString(String *string, const char* source, size_t len)
{
    assert(string && source);

    if (len < 1) return;

    while (string->count + len + 1 > string->capacity)
    {
        size_t oldCapacity = string->capacity;
        string->capacity = GROW_CAPACITY(oldCapacity);
        string->string = GROW_ARRAY(
            char, string->string, oldCapacity, string->capacity
        );
    }

    memcpy(string->string + string->count, source, len);
    string->count += len;
    string->string[string->count] = '\0';
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
initFromCharString(String* string, char* source)
{
    size_t len = strlen(source);
    string->string = source;
    string->count = len;
    string->capacity = len;
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
    while (string->count > 0 && isspace(string->string[string->count - 1]))
    {
        string->count--;
    }

    string->string[string->count] = '\0';
}
