#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "memory.h"

void*
reallocate(void* pointer, size_t oldSize, size_t newSize)
{
    if (newSize == 0)
    {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL)
    {
        perror("memory allocation");
        exit(1);
    }
    return result;
}
