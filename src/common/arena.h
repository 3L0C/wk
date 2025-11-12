#ifndef WK_COMMON_ARENA_H_
#define WK_COMMON_ARENA_H_

#include <stddef.h>

/* local includes */
#include "array.h"

#define ARENA_BLOCK_SIZE (1ULL << 16) /* 64KB blocks */
#define ARENA_ALIGNMENT 8
#define ARENA_ALIGN(n) (((n) + (ARENA_ALIGNMENT - 1)) & ~(ARENA_ALIGNMENT - 1))
#define ARENA_ALLOCATE(arena, type, count) (type*)arenaAlloc(arena, sizeof(type) * (count))
#define ARENA_ADOPT_ARRAY(arena, arr, type) (type*)arenaAdoptArray(arena, arr)

typedef struct Arena
{
    char*         buffer;
    size_t        bufferSize;
    size_t        used;
    struct Arena* prev;
} Arena;

void  arenaInit(Arena* arena);
void* arenaAdoptArray(Arena* arena, Array* arr);
void* arenaAlloc(Arena* arena, size_t size);
char* arenaCopyCString(Arena* arena, const char* src, size_t length);
void  arenaFree(Arena* arena);

#endif /* WK_COMMON_ARENA_H_ */
