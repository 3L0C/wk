#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* common includes */
#include "arena.h"
#include "array.h"
#include "string.h"

#define MIN(a, b) (a) < (b) ? (a) : (b);

static void
stringPartAdd(Arena* arena, String* string, const char* source, size_t length)
{
    assert(arena), assert(string), assert(source);

    StringPart part = {source, length};
    arrayAppend(&string->parts, &part);
    string->length += length;
}

void
stringAppendChar(Arena* arena, String* dest, char c)
{
    assert(arena), assert(dest);

    char* buffer = ARENA_ALLOCATE(arena, char, 2);
    buffer[0] = c;
    buffer[1] = '\0';
    stringPartAdd(arena, dest, buffer, 1);
}

void
stringAppendEscString(Arena* arena, String* dest, const char* source, size_t length)
{
    assert(arena), assert(dest), assert(source);

    for (size_t i = 0; i < length; i++)
    {
        size_t start = i;
        while (i < length && source[i] != '\\') i++;
        stringPartAdd(arena, dest, source + start, i - start);
    }
}

void
stringAppendInt32(Arena* arena, String* dest, int32_t i)
{
    assert(arena), assert(dest);

    size_t length = snprintf(NULL, 0, "%d", i);
    char* buffer = ARENA_ALLOCATE(arena, char, length + 1);
    snprintf(buffer, length + 1, "%d", i);
    stringPartAdd(arena, dest, buffer, length);
}

void
stringAppend(Arena* arena, String* dest, const char* source, size_t length)
{
    assert(arena), assert(dest), assert(source);
    if (length < 1) return;

    stringPartAdd(arena, dest, source, length);
}

void
stringAppendWithState(Arena* arena, String* dest, const char* source, size_t length, StringAppendState state)
{
    assert(arena), assert(dest), assert(source);
    if (length < 1) return;

    char* buffer;
    switch (state)
    {
    case STRING_APPEND_UPPER_FIRST:
    {
        stringAppendChar(arena, dest, toupper(*source));
        stringAppend(arena, dest, source + 1, length - 1);
        break;
    }
    case STRING_APPEND_LOWER_FIRST:
    {
        stringAppendChar(arena, dest, tolower(*source));
        stringAppend(arena, dest, source + 1, length - 1);
        break;
    }
    case STRING_APPEND_UPPER_ALL:
    {
        buffer = ARENA_ALLOCATE(arena, char, length + 1);
        for (size_t i = 0; i < length; i++)
        {
            buffer[i] = toupper(source[i]);
        }
        buffer[length] = '\0';
        stringPartAdd(arena, dest, buffer, length);
        break;
    }
    case STRING_APPEND_LOWER_ALL:
    {
        buffer = ARENA_ALLOCATE(arena, char, length + 1);
        for (size_t i = 0; i < length; i++)
        {
            buffer[i] = tolower(source[i]);
        }
        buffer[length] = '\0';
        stringPartAdd(arena, dest, buffer, length);
        break;
    }
    default: break;
    }
}

void
stringAppendUInt32(Arena* arena, String* dest, uint32_t i)
{
    assert(arena), assert(dest);

    size_t length = snprintf(NULL, 0, "%u", i);
    char* buffer = ARENA_ALLOCATE(arena, char, length + 1);
    snprintf(buffer, length + 1, "%u", i);
    stringPartAdd(arena, dest, buffer, length);
}

void
stringDisown(String* string)
{
    assert(string);

    stringInit(string);
}

bool
stringEquals(const String* a, const String* b)
{
    assert(a), assert(b);
    if (a == b) return true;
    if (a->length != b->length) return false;

    const Array* aParts = &a->parts;
    const Array* bParts = &b->parts;
    size_t aPartIndex = 0;
    size_t bPartIndex = 0;
    size_t aPartOffset = 0;
    size_t bPartOffset = 0;

    while (aPartIndex < aParts->length && bPartIndex < bParts->length)
    {
        const StringPart* aPart = ARRAY_GET(aParts, StringPart, aPartIndex);
        const StringPart* bPart = ARRAY_GET(bParts, StringPart, bPartIndex);

        size_t cmpLength = MIN(aPart->length - aPartOffset, bPart->length - bPartOffset);
        if (memcmp(aPart->source + aPartOffset, bPart->source + bPartOffset, cmpLength) != 0)
        {
            return false;
        }

        aPartOffset += cmpLength;
        bPartOffset += cmpLength;

        if (aPartOffset >= aPart->length)
        {
            aPartIndex++;
            aPartOffset = 0;
        }
        if (bPartOffset >= bPart->length)
        {
            bPartIndex++;
            bPartOffset = 0;
        }
    }

    return true;
}

void
stringFree(String* string)
{
    assert(string);

    stringDisown(string);
}

void
stringInitFromChar(Arena* arena, String* string, const char* source)
{
    assert(arena), assert(string), assert(source);

    stringInit(string);
    size_t length = strlen(source);
    stringPartAdd(arena, string, source, length);
}

void
stringInit(String* string)
{
    assert(string);

    string->parts = ARRAY_INIT(StringPart);
    string->length = 0;
}

bool
stringIsEmpty(const String* string)
{
    assert(string);

    return string->length == 0;
}


void
stringPrint(const String* string)
{
    assert(string);

    stringPrintToFile(string, stdout);
}

void
stringPrintToFile(const String* string, FILE* s)
{
    assert(string);
    if (s == NULL) return;

    ArrayIterator iter = arrayIteratorMake(&string->parts);
    const StringPart* part = NULL;
    while ((part = ARRAY_ITER_NEXT(&iter, const StringPart)) != NULL)
    {
        fwrite(part->source, sizeof(char), part->length, s);
    }
}


void
stringRtrim(String* string)
{
    assert(string);

    size_t i = string->parts.length;
    while (i-- > 0)
    {
        StringPart* part = ARRAY_GET(&string->parts, StringPart, i);
        size_t newLength = part->length;

        while (newLength > 0 && isspace(part->source[newLength - 1]))
        {
            newLength--;
            string->length--;
        }

        if (newLength != part->length)
        {
            part->length = newLength;
            if (newLength > 0) return;
        }

        arrayRemove(&string->parts, i);
    }
    string->length = 0;
}

size_t
stringLength(const String* string)
{
    assert(string);

    return string->length;
}

char*
stringToCString(Arena* arena, const String* string)
{
    assert(arena), assert(string);

    char* buffer = ARENA_ALLOCATE(arena, char, string->length + 1);
    stringWriteToBuffer(string, buffer);
    return buffer;
}

void
stringWriteToBuffer(const String* string, char* buffer)
{
    assert(string), assert(buffer);

    char* ptr = buffer;
    ArrayIterator iter = arrayIteratorMake(&string->parts);
    while (arrayIteratorHasNext(&iter))
    {
        const StringPart* part = ARRAY_ITER_NEXT(&iter, const StringPart);
        memcpy(ptr, part->source, part->length);
        ptr += part->length;
    }

    *ptr = '\0';
}

bool
stringIteratorHasNext(const StringIterator* iter)
{
    assert(iter);
    if (iter->partIndex < iter->string->parts.length)
    {
        StringPart* part = ARRAY_GET(&iter->string->parts, StringPart, iter->partIndex);
        if (iter->offset < part->length) return true;
        return (iter->partIndex + 1) < iter->string->parts.length;
    }
    return false;
}

void
stringIteratorInit(const String* string, StringIterator* iter)
{
    assert(string), assert(iter);

    iter->string = string;
    iter->partIndex = 0;
    iter->offset = 0;
}

StringIterator
stringIteratorMake(const String* string)
{
    assert(string);

    return (StringIterator){
        .string = string,
        .partIndex = 0,
        .offset = 0
    };
}

char
stringIteratorNext(StringIterator* iter)
{
    assert(iter), assert(iter->string);

    const Array* parts = &iter->string->parts;

    while (iter->partIndex < parts->length)
    {
        const StringPart* part = ARRAY_GET(parts, const StringPart, iter->partIndex);

        if (iter->offset < part->length)
        {
            return part->source[iter->offset++];
        }

        iter->partIndex++;
        iter->offset = 0;
    }

    return '\0';
}

char
stringIteratorPeek(const StringIterator* iter)
{
    assert(iter);

    StringIterator tmp = *iter;
    return stringIteratorNext(&tmp);
}

