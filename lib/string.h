#ifndef WK_LIB_STRING_H_
#define WK_LIB_STRING_H_

#include "common.h"

typedef struct
{
    char* string;
    size_t count;
    size_t capacity;
} String;

void appendToString(String* string, const char* source, size_t len);
void initString(String* string);
void freeString(String* string);

#endif /* WK_LIB_STRING_H_ */
