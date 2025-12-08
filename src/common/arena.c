#include <assert.h>
#include <stddef.h>
#include <string.h>

/* local includes */
#include "arena.h"
#include "memory.h"
#include "vector.h"

void
arenaInit(Arena* arena)
{
    arena->buffer     = NULL;
    arena->bufferSize = 0;
    arena->used       = 0;
    arena->prev       = NULL;
}

void*
arenaAdoptVector(Arena* arena, Vector* vec)
{
    assert(arena), assert(vec);
    if (vectorIsEmpty(vec)) return NULL;

    size_t bytes  = vectorLength(vec) * vec->elementSize;
    void*  buffer = arenaAlloc(arena, bytes);
    memcpy(buffer, vec->data, bytes);
    vectorFree(vec);
    return buffer;
}

void*
arenaAlloc(Arena* arena, size_t size)
{
    assert(arena);

    size = ARENA_ALIGN(size);

    if (arena->buffer == NULL || arena->used + size > arena->bufferSize)
    {
        size_t blockSize = size > ARENA_BLOCK_SIZE ? size : ARENA_BLOCK_SIZE;

        if (arena->buffer != NULL)
        {
            Arena* oldArena      = (Arena*)reallocate(NULL, 0, sizeof(Arena));
            oldArena->buffer     = arena->buffer;
            oldArena->bufferSize = arena->bufferSize;
            oldArena->used       = arena->used;
            oldArena->prev       = arena->prev;

            arena->prev = oldArena;
        }

        arena->buffer     = (char*)reallocate(NULL, 0, blockSize);
        arena->bufferSize = blockSize;
        arena->used       = size;

        return arena->buffer;
    }

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

    Arena* original = arena;

    while (arena != NULL)
    {
        Arena* prev = arena->prev;
        reallocate(arena->buffer, arena->bufferSize, 0);

        if (arena == original)
        {
            arena->buffer     = NULL;
            arena->bufferSize = 0;
            arena->used       = 0;
            arena->prev       = NULL;
        }
        else
        {
            reallocate(arena, sizeof(Arena), 0);
        }
        arena = prev;
    }
}
