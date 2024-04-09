#ifndef WK_LIB_STRING_H_
#define WK_LIB_STRING_H_

#include <stddef.h>

typedef struct
{
    char* string;
    size_t count;
    size_t capacity;
} String;

void appendToString(String* string, const char* source, size_t len);
void freeString(String* string);
void initFromCharString(String* string, char* source);
void initString(String* string);
void rtrimString(String* string);

#endif /* WK_LIB_STRING_H_ */
