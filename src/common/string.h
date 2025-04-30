#ifndef WK_COMMON_STRING_H_
#define WK_COMMON_STRING_H_

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

typedef uint8_t StringCase;
enum
{
    STRING_CASE_UPPER_FIRST,
    STRING_CASE_LOWER_FIRST,
    STRING_CASE_UPPER_ALL,
    STRING_CASE_LOWER_ALL
};

typedef struct
{
    const String* string;
    size_t partIndex;
    size_t offset;
} StringIterator;

void stringAppend(String* dest, const char* src, size_t len);
void stringAppendChar(Arena* arena, String* dest, char c);
void stringAppendCString(String* dest, const char* src);
void stringAppendEscString(String* dest, const char* src, size_t len);
void stringAppendInt32(Arena* arena, String* dest, int32_t i);
void stringAppendString(String* dest, const String* src);
void stringAppendStringWithState(Arena* arena, String* dest, const String* src, StringCase state);
void stringAppendUInt32(Arena* arena, String* dest, uint32_t i);
void stringAppendWithState(Arena* arena, String* dest, const char* src, size_t len, StringCase state);
int  stringCompare(const String* a, const String* b);
String stringCopy(const String* from);
bool stringEquals(const String* a, const String* b);
void stringFree(String* string);
String stringInitFromChar(const char* src);
String stringInit(void);
bool stringIsEmpty(const String* string);
String stringMake(Arena* arena, const char* src);
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
