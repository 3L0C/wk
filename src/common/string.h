#ifndef WK_LIB_STRING_H_
#define WK_LIB_STRING_H_

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    char* string;
    size_t count;
    size_t capacity;
} String;

void appendInt32ToString(String* string, int32_t i);
void appendUInt32ToString(String* string, uint32_t i);
void appendCharToString(String* string, char c);
void appendToString(String* string, const char* source, size_t len);
void disownString(String* string);
void freeString(String* string);
void initFromCharString(String* string, char* source);
void initString(String* string);
void rtrimString(String* string);

#endif /* WK_LIB_STRING_H_ */
