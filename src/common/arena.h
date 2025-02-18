#ifndef WK_COMMON_ARENA_H_
#define WK_COMMON_ARENA_H_

#include <stddef.h>

#define ARENA_BLOCK_SIZE (1ULL << 16) /* 64KB blocks */
#define ARENA_ALIGNMENT 8
#define ARENA_ALIGN(n) (((n) + (ARENA_ALIGNMENT - 1)) & ~(ARENA_ALIGNMENT - 1))
#define ARENA_ALLOCATE(arena, type, count) (type*)arenaAlloc(arena, sizeof(type) * (count))

typedef struct Arena
{
    char*  buffer;
    size_t bufferSize;
    size_t used;
    struct Arena* prev;
} Arena;

void  arenaInit(Arena* arena);
void* arenaAlloc(Arena* arena, size_t size);
void  arenaFree(Arena* arena);

#endif /* WK_COMMON_ARENA_H_ */
