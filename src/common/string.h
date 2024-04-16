#ifndef WK_COMMON_STRING_H_
#define WK_COMMON_STRING_H_

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    char* string;
    size_t count;
    size_t capacity;
} String;

typedef enum
{
    STRING_APPEND_UPPER_FIRST,
    STRING_APPEND_LOWER_FIRST,
    STRING_APPEND_UPPER_ALL,
    STRING_APPEND_LOWER_ALL,
} StringAppendState;

void appendInt32ToString(String* dest, int32_t i);
void appendUInt32ToString(String* dest, uint32_t i);
void appendCharToString(String* dest, char c);
void appendToString(String* dest, const char* source, size_t len);
void appendToStringWithState(String* dest, const char* source, size_t len, StringAppendState state);
void disownString(String* string);
void freeString(String* string);
void initFromCharString(String* string, char* source);
void initString(String* string);
void rtrimString(String* string);

#endif /* WK_COMMON_STRING_H_ */
