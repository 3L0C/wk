#ifndef WK_COMMON_STRING_H_
#define WK_COMMON_STRING_H_

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* local includes */
#include "arena.h"
#include "array.h"

typedef struct StringPart
{
    const char* source;
    size_t length;
} StringPart;

typedef struct
{
    Array parts;
    size_t length;
} String;

typedef enum
{
    STRING_APPEND_UPPER_FIRST,
    STRING_APPEND_LOWER_FIRST,
    STRING_APPEND_UPPER_ALL,
    STRING_APPEND_LOWER_ALL
} StringAppendState;

typedef struct
{
    const String* string;
    size_t partIndex;
    size_t offset;
} StringIterator;

void stringAppendChar(Arena* arena, String* dest, char c);
void stringAppendEscString(Arena* arena, String* dest, const char* source, size_t len);
void stringAppendInt32(Arena* arena, String* dest, int32_t i);
void stringAppend(Arena* arena, String* dest, const char* source, size_t len);
void stringAppendWithState(Arena* arena, String* dest, const char* source, size_t len, StringAppendState state);
void stringAppendUInt32(Arena* arena, String* dest, uint32_t i);
void stringDisown(String* string);
bool stringEquals(const String* a, const String* b);
void stringFree(String* string);
void stringInitFromChar(Arena* arena, String* string, const char* source);
void stringInit(String* string);
bool stringIsEmpty(const String* string);
void stringPrint(const String* string);
void stringPrintToFile(const String* string, FILE* s);
void stringRtrim(String* string);
size_t stringLength(const String* string);
char* stringToCString(Arena* arena, const String* string);
void stringWriteToBuffer(const String* string, char* buffer);

void stringIteratorAdvance(StringIterator* iter);
bool stringIteratorHasNext(const StringIterator* iter);
void stringIteratorInit(const String* string, StringIterator* iter);
StringIterator stringIteratorMake(const String* string);
char stringIteratorNext(StringIterator* iter);
char stringIteratorPeek(const StringIterator* iter);

#endif /* WK_COMMON_STRING_H_ */
