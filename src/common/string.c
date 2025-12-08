#include <assert.h>
#include <string.h>

/* local includes */
#include "string.h"

bool
stringEquals(const String* a, const String* b)
{
    /* Two NULL pointers are considered equal */
    if (a == NULL || b == NULL) return a == b;
    if (a->length != b->length) return false;
    if (a->data == b->data) return true;
    if (a->data == NULL || b->data == NULL) return a->data == b->data;

    return memcmp(a->data, b->data, a->length) == 0;
}

String
stringFromCString(Arena* arena, const char* src)
{
    assert(arena);

    if (src == NULL) return (String){ .data = NULL, .length = 0 };

    return stringMake(arena, src, strlen(src));
}

bool
stringIsEmpty(const String* str)
{
    return str == NULL || str->data == NULL || str->length == 0;
}

String
stringMake(Arena* arena, const char* src, size_t len)
{
    assert(arena);

    String str;
    if (src == NULL || len == 0)
    {
        str.data   = NULL;
        str.length = 0;
        return str;
    }

    str.data   = arenaCopyCString(arena, src, len);
    str.length = len;
    return str;
}
