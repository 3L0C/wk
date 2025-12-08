#ifndef WK_COMPILER_LAZY_STRING_H_
#define WK_COMPILER_LAZY_STRING_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* common includes */
#include "common/arena.h"
#include "common/string.h"
#include "common/vector.h"

typedef struct LazyStringPart
{
    const char* source;
    size_t      length;
} LazyStringPart;

typedef struct
{
    Vector parts;
    size_t length;
} LazyString;

typedef uint8_t LazyStringCase;
enum
{
    LAZY_STRING_CASE_UPPER_FIRST,
    LAZY_STRING_CASE_LOWER_FIRST,
    LAZY_STRING_CASE_UPPER_ALL,
    LAZY_STRING_CASE_LOWER_ALL
};

typedef struct
{
    const LazyString* string;
    size_t            partIndex;
    size_t            offset;
} LazyStringIterator;

void       lazyStringAppend(LazyString* dest, const char* src, size_t len);
void       lazyStringAppendChar(Arena* arena, LazyString* dest, char c);
void       lazyStringAppendCString(LazyString* dest, const char* src);
void       lazyStringAppendEscString(LazyString* dest, const char* src, size_t len);
void       lazyStringAppendInt32(Arena* arena, LazyString* dest, int32_t i);
void       lazyStringAppendLazyString(LazyString* dest, const LazyString* src);
void       lazyStringAppendLazyStringWithState(Arena* arena, LazyString* dest, const LazyString* src, LazyStringCase state);
void       lazyStringAppendUInt32(Arena* arena, LazyString* dest, uint32_t i);
void       lazyStringAppendWithState(Arena* arena, LazyString* dest, const char* src, size_t len, LazyStringCase state);
int        lazyStringCompare(const LazyString* a, const LazyString* b);
LazyString lazyStringCopy(const LazyString* from);
bool       lazyStringEquals(const LazyString* a, const LazyString* b);
void       lazyStringFree(LazyString* string);
LazyString lazyStringInitFromChar(const char* src);
LazyString lazyStringInit(void);
bool       lazyStringIsEmpty(const LazyString* string);
LazyString lazyStringMake(Arena* arena, const char* src);
void       lazyStringPrint(const LazyString* string);
void       lazyStringPrintToFile(const LazyString* string, FILE* s);
void       lazyStringRtrim(LazyString* string);
size_t     lazyStringLength(const LazyString* string);
char*      lazyStringToCString(Arena* arena, const LazyString* string);
void       lazyStringWriteToBuffer(const LazyString* string, char* buffer);
String     lazyStringToString(Arena* arena, const LazyString* string);

void               lazyStringIteratorAdvance(LazyStringIterator* iter);
bool               lazyStringIteratorHasNext(const LazyStringIterator* iter);
void               lazyStringIteratorInit(const LazyString* string, LazyStringIterator* iter);
LazyStringIterator lazyStringIteratorMake(const LazyString* string);
char               lazyStringIteratorNext(LazyStringIterator* iter);
char               lazyStringIteratorPeek(const LazyStringIterator* iter);

#endif /* WK_COMPILER_LAZY_STRING_H_ */
