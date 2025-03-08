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
stringPartAdd(String* string, const char* src, size_t length)
{
    assert(string), assert(src);

    StringPart part = {src, length};
    arrayAppend(&string->parts, &part);
    string->length += length;
}

void
stringAppend(String* dest, const char* src, size_t length)
{
    assert(dest), assert(src);
    if (length < 1) return;

    stringPartAdd(dest, src, length);
}

void
stringAppendChar(Arena* arena, String* dest, char c)
{
    assert(arena), assert(dest);

    char* buffer = ARENA_ALLOCATE(arena, char, 2);
    buffer[0] = c;
    buffer[1] = '\0';
    stringPartAdd(dest, buffer, 1);
}

void
stringAppendCString(String* dest, const char* src)
{
    assert(dest), assert(src);

    size_t length = strlen(src);
    stringAppend(dest, src, length);
}

void
stringAppendEscString(String* dest, const char* src, size_t length)
{
    assert(dest), assert(src);

    for (size_t i = 0; i < length; i++)
    {
        size_t start = i;
        while (i < length && src[i] != '\\') i++;
        stringPartAdd(dest, src + start, i - start);
    }
}

void
stringAppendInt32(Arena* arena, String* dest, int32_t i)
{
    assert(arena), assert(dest);

    size_t length = snprintf(NULL, 0, "%d", i);
    char* buffer = ARENA_ALLOCATE(arena, char, length + 1);
    snprintf(buffer, length + 1, "%d", i);
    stringPartAdd(dest, buffer, length);
}

void
stringAppendString(String* dest, const String* src)
{
    assert(dest), assert(src);
    if (src->length == 0 || src->parts.length == 0) return;

    arrayAppendN(&dest->parts, src->parts.data, src->parts.length);
    dest->length += src->length;
}

void
stringAppendStringWithState(Arena* arena, String* dest, const String* src, StringCase state)
{
    assert(arena), assert(dest), assert(src);
    if (src->length == 0) return;

    const Array* srcParts = &src->parts;

    switch (state)
    {
    case STRING_CASE_UPPER_FIRST: /* FALLTHROUGH */
    case STRING_CASE_LOWER_FIRST:
    {
        StringPart* firstPart = ARRAY_GET(srcParts, StringPart, 0);
        char transformed = (state == STRING_CASE_UPPER_FIRST)
            ? toupper(*firstPart->source)
            : tolower(*firstPart->source);
        stringAppendChar(arena, dest, transformed);

        if (firstPart->length > 1)
        {
            StringPart remaining = { firstPart->source + 1, firstPart->length - 1 };
            arrayAppend(&dest->parts, &remaining);
        }

        if (srcParts->length > 1)
        {
            arrayAppendN(&dest->parts, ARRAY_GET(srcParts, StringPart, 1), srcParts->length - 1);
        }

        dest->length += src->length - 1;
        break;
    }
    case STRING_CASE_UPPER_ALL: /* FALLTHROUGH */
    case STRING_CASE_LOWER_ALL:
    {
        char* buffer = ARENA_ALLOCATE(arena, char, src->length + 1);
        char* ptr = buffer;

        forEach(srcParts, StringPart, part)
        {
            const char* srcPtr = part->source;
            for (size_t j = 0; j < part->length; j++)
            {
                *ptr++ = (state == STRING_CASE_UPPER_ALL)
                    ? toupper(srcPtr[j])
                    : tolower(srcPtr[j]);
            }
        }
        *ptr = '\0';

        stringAppend(dest, buffer, src->length);
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
    stringPartAdd(dest, buffer, length);
}

void
stringAppendWithState(Arena* arena, String* dest, const char* src, size_t length, StringCase state)
{
    assert(arena), assert(dest), assert(src);
    if (length < 1) return;

    switch (state)
    {
    case STRING_CASE_UPPER_FIRST: /* FALLTHROUGH */
    case STRING_CASE_LOWER_FIRST:
    {
        char c = (state == STRING_CASE_UPPER_FIRST) ? toupper(*src) : tolower(*src);
        stringAppendChar(arena, dest, c);
        if (length > 1) stringAppend(dest, src + 1, length - 1);
        break;
    }
    case STRING_CASE_UPPER_ALL: /* FALLTHROUGH */
    case STRING_CASE_LOWER_ALL:
    {
        char* buffer = ARENA_ALLOCATE(arena, char, length + 1);
        char* ptr = buffer;
        for (size_t i = 0; i < length; i++)
        {
            *ptr++ = (state == STRING_CASE_UPPER_ALL)
                ? toupper(*src++)
                : tolower(*src++);
        }
        *ptr = '\0';

        stringPartAdd(dest, buffer, length);
        break;
    }
    default: break;
    }
}

int
stringCompare(const String* a, const String* b)
{
    assert(a), assert(b);
    if (a == b) return 0;

    const Array* aParts = &a->parts;
    const Array* bParts = &b->parts;
    size_t aPartIndex = 0;
    size_t bPartIndex = 0;
    size_t aPartOffset = 0;
    size_t bPartOffset = 0;

    while (aPartIndex < aParts->length && bPartIndex < bParts->length) {
        const StringPart* aPart = ARRAY_GET(aParts, StringPart, aPartIndex);
        const StringPart* bPart = ARRAY_GET(bParts, StringPart, bPartIndex);

        size_t cmpLength = MIN(aPart->length - aPartOffset, bPart->length - bPartOffset);
        int cmp = memcmp(aPart->source + aPartOffset, bPart->source + bPartOffset, cmpLength);
        if (cmp != 0) return cmp;

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

    if (a->length < b->length)      return -1;
    else if (a->length > b->length) return 1;
    else                            return 0;
}

String
stringCopy(const String* from)
{
    assert(from);

    String to = stringInit();
    if (from->length > 0 && from->parts.length > 0)
    {
        arrayAppendN(&to.parts, from->parts.data, from->parts.length);
    }

    to.length = from->length;
    return to;
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

    arrayFree(&string->parts);
    string->parts = ARRAY_INIT(StringPart);
    string->length = 0;
}

String
stringInitFromChar(const char* src)
{
    assert(src);

    String string = stringInit();
    size_t length = strlen(src);
    stringPartAdd(&string, src, length);
    return string;
}

String
stringInit(void)
{
    return (String){
        .parts = ARRAY_INIT(StringPart),
        .length = 0
    };
}

bool
stringIsEmpty(const String* string)
{
    assert(string);

    return string->length == 0;
}

String
stringMake(Arena* arena, const char* src)
{
    assert(arena), assert(src);

    String result = stringInit();

    size_t length = strlen(src);
    if (length == 0)
    {
        return result;
    }

    char* buffer = ARENA_ALLOCATE(arena, char, length + 1);
    memcpy(buffer, src, length);
    buffer[length] = '\0';
    stringPartAdd(&result, buffer, length);
    return result;
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

        if (newLength == part->length) return;

        part->length = newLength;
        if (newLength > 0) return;

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

