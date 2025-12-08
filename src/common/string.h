#ifndef WK_COMMON_STRING_H_
#define WK_COMMON_STRING_H_

#include <stdbool.h>
#include <stddef.h>

/* local includes */
#include "arena.h"

/* null-terminated, arena-owned */
typedef struct
{
    const char* data;
    size_t      length;
} String;

bool   stringEquals(const String* a, const String* b);
String stringFromCString(Arena* arena, const char* src);
bool   stringIsEmpty(const String* str);
String stringMake(Arena* arena, const char* src, size_t len);

#endif /* WK_COMMON_STRING_H_ */
