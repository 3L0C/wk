#ifndef WK_COMMON_SPAN_H_
#define WK_COMMON_SPAN_H_

#include <stddef.h>
#include <string.h>

/* local includes */
#include "arena.h"
#include "vector.h"

typedef struct
{
    void*  data;
    size_t count;
} Span;

#define SPAN_GET(span, type, index) \
    ((type*)spanGet((span), sizeof(type), (index)))
#define SPAN_AS(span, type) ((type*)(span)->data)
#define SPAN_LENGTH(span) ((span)->count)
#define SPAN_EMPTY ((Span){ .data = NULL, .count = 0 })

#define spanForEach(span, type, var)                                    \
    for (size_t _i = 0; _i < (span)->count; _i++)                       \
        for (type* var = &((type*)(span)->data)[_i], *_once = (type*)1; \
             _once;                                                     \
             _once = NULL)

#define SPAN_FROM_VECTOR(arena, vec, type) spanFromVector((arena), (vec), sizeof(type))

static inline Span
spanFromVector(Arena* arena, Vector* vec, size_t elementSize)
{
    assert(arena), assert(vec);

    if (vectorIsEmpty(vec))
    {
        vectorFree(vec);
        return (Span){ .data = NULL, .count = 0 };
    }

    size_t count = vectorLength(vec);
    size_t bytes = count * elementSize;
    void*  data  = arenaAlloc(arena, bytes);
    memcpy(data, vec->data, bytes);
    vectorFree(vec);

    return (Span){ .data = data, .count = count };
}

static inline void*
spanGet(const Span* span, size_t elementSize, size_t index)
{
    assert(span);

    if (index >= span->count) return NULL;
    return (char*)span->data + (index * elementSize);
}

static inline Span
spanMake(void* data, size_t count)
{
    return (Span){ .data = data, .count = count };
}

#endif /* WK_COMMON_SPAN_H_ */
