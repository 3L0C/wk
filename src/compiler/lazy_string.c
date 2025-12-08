#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* common includes */
#include "common/arena.h"
#include "common/string.h"
#include "common/vector.h"

/* local includes */
#include "lazy_string.h"

#define MIN(a, b) (a) < (b) ? (a) : (b);

static void
lazyStringPartAdd(LazyString* string, const char* src, size_t length)
{
    assert(string), assert(src);

    LazyStringPart part = { src, length };
    vectorAppend(&string->parts, &part);
    string->length += length;
}

void
lazyStringAppend(LazyString* dest, const char* src, size_t length)
{
    assert(dest), assert(src);
    if (length < 1) return;

    lazyStringPartAdd(dest, src, length);
}

void
lazyStringAppendChar(Arena* arena, LazyString* dest, char c)
{
    assert(arena), assert(dest);

    char* buffer = ARENA_ALLOCATE(arena, char, 2);
    buffer[0]    = c;
    buffer[1]    = '\0';
    lazyStringPartAdd(dest, buffer, 1);
}

void
lazyStringAppendCString(LazyString* dest, const char* src)
{
    assert(dest), assert(src);

    size_t length = strlen(src);
    lazyStringAppend(dest, src, length);
}

void
lazyStringAppendEscString(LazyString* dest, const char* src, size_t length)
{
    assert(dest), assert(src);

    for (size_t i = 0; i < length; i++)
    {
        size_t start = i;
        while (i < length && src[i] != '\\')
            i++;
        lazyStringPartAdd(dest, src + start, i - start);
    }
}

void
lazyStringAppendInt32(Arena* arena, LazyString* dest, int32_t i)
{
    assert(arena), assert(dest);

    size_t length = snprintf(NULL, 0, "%d", i);
    char*  buffer = ARENA_ALLOCATE(arena, char, length + 1);
    snprintf(buffer, length + 1, "%d", i);
    lazyStringPartAdd(dest, buffer, length);
}

void
lazyStringAppendLazyString(LazyString* dest, const LazyString* src)
{
    assert(dest), assert(src);
    if (src->length == 0 || src->parts.length == 0) return;

    vectorAppendN(&dest->parts, src->parts.data, src->parts.length);
    dest->length += src->length;
}

void
lazyStringAppendLazyStringWithState(Arena* arena, LazyString* dest, const LazyString* src, LazyStringCase state)
{
    assert(arena), assert(dest), assert(src);
    if (src->length == 0) return;

    const Vector* srcParts = &src->parts;

    switch (state)
    {
    case LAZY_STRING_CASE_UPPER_FIRST: /* FALLTHROUGH */
    case LAZY_STRING_CASE_LOWER_FIRST:
    {
        LazyStringPart* firstPart   = VECTOR_GET(srcParts, LazyStringPart, 0);
        char            transformed = (state == LAZY_STRING_CASE_UPPER_FIRST)
                                          ? toupper(*firstPart->source)
                                          : tolower(*firstPart->source);
        lazyStringAppendChar(arena, dest, transformed);

        if (firstPart->length > 1)
        {
            LazyStringPart remaining = { firstPart->source + 1, firstPart->length - 1 };
            vectorAppend(&dest->parts, &remaining);
        }

        if (srcParts->length > 1)
        {
            vectorAppendN(&dest->parts, VECTOR_GET(srcParts, LazyStringPart, 1), srcParts->length - 1);
        }

        dest->length += src->length - 1;
        break;
    }
    case LAZY_STRING_CASE_UPPER_ALL: /* FALLTHROUGH */
    case LAZY_STRING_CASE_LOWER_ALL:
    {
        char* buffer = ARENA_ALLOCATE(arena, char, src->length + 1);
        char* ptr    = buffer;

        vectorForEach(srcParts, LazyStringPart, part)
        {
            const char* srcPtr = part->source;
            for (size_t j = 0; j < part->length; j++)
            {
                *ptr++ = (state == LAZY_STRING_CASE_UPPER_ALL)
                             ? toupper(srcPtr[j])
                             : tolower(srcPtr[j]);
            }
        }
        *ptr = '\0';

        lazyStringAppend(dest, buffer, src->length);
        break;
    }
    default: break;
    }
}

void
lazyStringAppendUInt32(Arena* arena, LazyString* dest, uint32_t i)
{
    assert(arena), assert(dest);

    size_t length = snprintf(NULL, 0, "%u", i);
    char*  buffer = ARENA_ALLOCATE(arena, char, length + 1);
    snprintf(buffer, length + 1, "%u", i);
    lazyStringPartAdd(dest, buffer, length);
}

void
lazyStringAppendWithState(Arena* arena, LazyString* dest, const char* src, size_t length, LazyStringCase state)
{
    assert(arena), assert(dest), assert(src);
    if (length < 1) return;

    switch (state)
    {
    case LAZY_STRING_CASE_UPPER_FIRST: /* FALLTHROUGH */
    case LAZY_STRING_CASE_LOWER_FIRST:
    {
        char c = (state == LAZY_STRING_CASE_UPPER_FIRST) ? toupper(*src) : tolower(*src);
        lazyStringAppendChar(arena, dest, c);
        if (length > 1) lazyStringAppend(dest, src + 1, length - 1);
        break;
    }
    case LAZY_STRING_CASE_UPPER_ALL: /* FALLTHROUGH */
    case LAZY_STRING_CASE_LOWER_ALL:
    {
        char* buffer = ARENA_ALLOCATE(arena, char, length + 1);
        char* ptr    = buffer;
        for (size_t i = 0; i < length; i++)
        {
            *ptr++ = (state == LAZY_STRING_CASE_UPPER_ALL)
                         ? toupper(*src++)
                         : tolower(*src++);
        }
        *ptr = '\0';

        lazyStringPartAdd(dest, buffer, length);
        break;
    }
    default: break;
    }
}

int
lazyStringCompare(const LazyString* a, const LazyString* b)
{
    assert(a), assert(b);
    if (a == b) return 0;

    const Vector* aParts      = &a->parts;
    const Vector* bParts      = &b->parts;
    size_t        aPartIndex  = 0;
    size_t        bPartIndex  = 0;
    size_t        aPartOffset = 0;
    size_t        bPartOffset = 0;

    while (aPartIndex < aParts->length && bPartIndex < bParts->length)
    {
        const LazyStringPart* aPart = VECTOR_GET(aParts, LazyStringPart, aPartIndex);
        const LazyStringPart* bPart = VECTOR_GET(bParts, LazyStringPart, bPartIndex);

        size_t cmpLength = MIN(aPart->length - aPartOffset, bPart->length - bPartOffset);
        int    cmp       = memcmp(
            aPart->source + aPartOffset,
            bPart->source + bPartOffset,
            cmpLength);
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

    if (a->length < b->length) return -1;
    else if (a->length > b->length) return 1;
    else return 0;
}

LazyString
lazyStringCopy(const LazyString* from)
{
    assert(from);

    LazyString to = lazyStringInit();
    if (!lazyStringIsEmpty(from))
    {
        to.parts = vectorCopy(&from->parts);
    }

    to.length = from->length;
    return to;
}

bool
lazyStringEquals(const LazyString* a, const LazyString* b)
{
    assert(a), assert(b);
    if (a == b) return true;
    if (a->length != b->length) return false;

    const Vector* aParts      = &a->parts;
    const Vector* bParts      = &b->parts;
    size_t        aPartIndex  = 0;
    size_t        bPartIndex  = 0;
    size_t        aPartOffset = 0;
    size_t        bPartOffset = 0;

    while (aPartIndex < aParts->length && bPartIndex < bParts->length)
    {
        const LazyStringPart* aPart = VECTOR_GET(aParts, LazyStringPart, aPartIndex);
        const LazyStringPart* bPart = VECTOR_GET(bParts, LazyStringPart, bPartIndex);

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
lazyStringFree(LazyString* string)
{
    assert(string);

    vectorFree(&string->parts);
    string->parts  = VECTOR_INIT(LazyStringPart);
    string->length = 0;
}

LazyString
lazyStringInit(void)
{
    return (LazyString){
        .parts  = VECTOR_INIT(LazyStringPart),
        .length = 0
    };
}

LazyString
lazyStringInitFromChar(const char* src)
{
    if (src == NULL) return lazyStringInit();

    LazyString string = lazyStringInit();
    size_t     length = strlen(src);
    lazyStringPartAdd(&string, src, length);
    return string;
}

bool
lazyStringIsEmpty(const LazyString* string)
{
    assert(string);

    return string->length == 0;
}

bool
lazyStringIteratorHasNext(const LazyStringIterator* iter)
{
    assert(iter);
    if (iter->partIndex < iter->string->parts.length)
    {
        LazyStringPart* part = VECTOR_GET(&iter->string->parts, LazyStringPart, iter->partIndex);
        if (iter->offset < part->length) return true;
        return (iter->partIndex + 1) < iter->string->parts.length;
    }
    return false;
}

void
lazyStringIteratorInit(const LazyString* string, LazyStringIterator* iter)
{
    assert(string), assert(iter);

    iter->string    = string;
    iter->partIndex = 0;
    iter->offset    = 0;
}

LazyStringIterator
lazyStringIteratorMake(const LazyString* string)
{
    assert(string);

    return (LazyStringIterator){
        .string    = string,
        .partIndex = 0,
        .offset    = 0
    };
}

char
lazyStringIteratorNext(LazyStringIterator* iter)
{
    assert(iter), assert(iter->string);

    const Vector* parts = &iter->string->parts;

    while (iter->partIndex < parts->length)
    {
        const LazyStringPart* part = VECTOR_GET(parts, const LazyStringPart, iter->partIndex);

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
lazyStringIteratorPeek(const LazyStringIterator* iter)
{
    assert(iter);

    LazyStringIterator tmp = *iter;
    return lazyStringIteratorNext(&tmp);
}

size_t
lazyStringLength(const LazyString* string)
{
    assert(string);

    return string->length;
}

LazyString
lazyStringMake(Arena* arena, const char* src)
{
    assert(arena), assert(src);

    LazyString result = lazyStringInit();

    size_t length = strlen(src);
    if (length == 0)
    {
        return result;
    }

    char* buffer = ARENA_ALLOCATE(arena, char, length + 1);
    memcpy(buffer, src, length);
    buffer[length] = '\0';
    lazyStringPartAdd(&result, buffer, length);
    return result;
}

void
lazyStringPrint(const LazyString* string)
{
    assert(string);

    lazyStringPrintToFile(string, stdout);
}

void
lazyStringPrintToFile(const LazyString* string, FILE* s)
{
    assert(string);
    if (s == NULL) return;

    vectorForEach(&string->parts, const LazyStringPart, part)
    {
        fwrite(part->source, sizeof(char), part->length, s);
    }
}

void
lazyStringRtrim(LazyString* string)
{
    assert(string);

    size_t i = string->parts.length;
    while (i-- > 0)
    {
        LazyStringPart* part      = VECTOR_GET(&string->parts, LazyStringPart, i);
        size_t          newLength = part->length;

        while (newLength > 0 && isspace(part->source[newLength - 1]))
        {
            newLength--;
            string->length--;
        }

        if (newLength == part->length) return;

        part->length = newLength;
        if (newLength > 0) return;

        vectorRemove(&string->parts, i);
    }
    string->length = 0;
}

char*
lazyStringToCString(Arena* arena, const LazyString* string)
{
    assert(arena), assert(string);

    char* buffer = ARENA_ALLOCATE(arena, char, string->length + 1);
    lazyStringWriteToBuffer(string, buffer);
    return buffer;
}

String
lazyStringToString(Arena* arena, const LazyString* string)
{
    assert(arena);

    if (string == NULL || lazyStringIsEmpty(string))
    {
        return (String){ .data = NULL, .length = 0 };
    }

    char* cstr = lazyStringToCString(arena, string);
    return (String){ .data = cstr, .length = lazyStringLength(string) };
}

void
lazyStringWriteToBuffer(const LazyString* string, char* buffer)
{
    assert(string), assert(buffer);

    char*          ptr  = buffer;
    VectorIterator iter = vectorIteratorMake(&string->parts);
    while (vectorIteratorHasNext(&iter))
    {
        const LazyStringPart* part = VECTOR_ITER_NEXT(&iter, const LazyStringPart);
        memcpy(ptr, part->source, part->length);
        ptr += part->length;
    }

    *ptr = '\0';
}
