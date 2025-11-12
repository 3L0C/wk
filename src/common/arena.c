#include <assert.h>
#include <stddef.h>
#include <string.h>

/* local includes */
#include "arena.h"
#include "array.h"
#include "memory.h"

void
arenaInit(Arena* arena)
{
    arena->buffer     = NULL;
    arena->bufferSize = 0;
    arena->used       = 0;
    arena->prev       = NULL;
}

void*
arenaAdoptArray(Arena* arena, Array* arr)
{
    assert(arena), assert(arr);
    if (arrayIsEmpty(arr)) return NULL;

    size_t bytes  = arrayLength(arr) * arr->elementSize;
    void*  buffer = arenaAlloc(arena, bytes);
    memcpy(buffer, arr->data, bytes);
    arrayFree(arr);
    return buffer;
}

void*
arenaAlloc(Arena* arena, size_t size)
{
    assert(arena);

    size = ARENA_ALIGN(size);

    /* If this allocation won't fit in the current block */
    if (arena->buffer == NULL || arena->used + size > arena->bufferSize)
    {
        /* Allocate a new block */
        size_t blockSize = size > ARENA_BLOCK_SIZE ? size : ARENA_BLOCK_SIZE;

        if (arena->buffer != NULL)
        {
            /* If we already have a buffer, create a new arena to hold the old state */
            Arena* oldArena      = (Arena*)reallocate(NULL, 0, sizeof(Arena));
            oldArena->buffer     = arena->buffer;
            oldArena->bufferSize = arena->bufferSize;
            oldArena->used       = arena->used;
            oldArena->prev       = arena->prev;

            /* Update the current arena */
            arena->prev = oldArena;
        }

        /* Allocate new buffer for the current arena */
        arena->buffer     = (char*)reallocate(NULL, 0, blockSize);
        arena->bufferSize = blockSize;
        arena->used       = size;

        return arena->buffer;
    }

    /* Allocation fits in current block */
    void* result = arena->buffer + arena->used;
    arena->used += size;

    return result;
}

char*
arenaCopyCString(Arena* arena, const char* src, size_t length)
{
    assert(arena), assert(src);

    char* result = ARENA_ALLOCATE(arena, char, length + 1);
    memcpy(result, src, length);
    result[length] = '\0';

    return result;
}

void
arenaFree(Arena* arena)
{
    assert(arena);

    while (arena != NULL)
    {
        Arena* prev = arena->prev;
        reallocate(arena->buffer, arena->bufferSize, 0);

        if (prev == NULL)
        {
            /* This is the Last arena - reset it instead of freeing */
            arena->buffer     = NULL;
            arena->bufferSize = 0;
            arena->used       = 0;
            return;
        }

        reallocate(arena, sizeof(Arena), 0);
        arena = prev;
    }
}
