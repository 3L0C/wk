#ifndef WK_COMMON_VECTOR_H_
#define WK_COMMON_VECTOR_H_

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* local includes */
#include "memory.h"

#define VECTOR_INIT(type) vectorInit(sizeof(type))
#define VECTOR_GET(vec, type, index) ((type*)vectorGet((vec), (index)))
#define VECTOR_GET_LAST(vec, type) VECTOR_GET(vec, type, vectorLastIndex(vec))
#define VECTOR_AS(vec, type) VECTOR_GET(vec, type, 0)
#define VECTOR_GROW(ptr, oldCapacity, newCapacity, elementSize) \
    reallocate((ptr), (elementSize) * (oldCapacity), (elementSize) * (newCapacity))
#define VECTOR_APPEND_SLOT(vec, type) (vectorGrowForAppend(vec), VECTOR_GET_LAST(vec, type))
#define CAPACITY_GROW(size) (size) == 0 ? 8 : (size) * 2
#define VECTOR_ITER_NEXT(iter, type) ((type*)vectorIteratorNext(iter))
#define VECTOR_ITER_PEEK(iter, type) ((type*)vectorIteratorPeek(iter))
#define vectorForRange(vec, type, var, startIdx, endIdx)                                   \
    for (VectorIterator iter = vectorIteratorMakeStartingAt(vec, startIdx);                \
         (iter.index == (size_t)(startIdx) - 1) ? (vectorIteratorHasNext(&iter)) : false;) \
        for (type* var = NULL;                                                             \
             (var = VECTOR_ITER_NEXT(&iter, type)) != NULL && iter.index < (size_t)(endIdx);)
#define vectorForEachFrom(vec, type, var, startIdx) vectorForRange(vec, type, var, startIdx, vectorLength(vec))
#define vectorForEach(vec, type, var) vectorForRange(vec, type, var, 0, vectorLength(vec))

typedef struct
{
    void*  data;
    size_t length;
    size_t capacity;
    size_t elementSize;
} Vector;

typedef struct
{
    const Vector* vec;
    size_t        index;
} VectorIterator;

static inline void
vectorAppend(Vector* vec, const void* value)
{
    assert(vec), assert(value);

    if (vec->length >= vec->capacity)
    {
        const size_t oldCapacity = vec->capacity;

        vec->capacity = CAPACITY_GROW(oldCapacity);
        vec->data     = VECTOR_GROW(vec->data, oldCapacity, vec->capacity, vec->elementSize);
    }

    void* dest = (char*)vec->data + (vec->length * vec->elementSize);
    memcpy(dest, value, vec->elementSize);
    vec->length++;
}

static inline void
vectorAppendN(Vector* vec, const void* value, size_t n)
{
    assert(vec), assert(value);
    if (n == 0) return;

    size_t required = vec->length + n;
    if (required > vec->capacity)
    {
        size_t oldCapacity = vec->capacity;
        size_t newCapacity = CAPACITY_GROW(oldCapacity);
        if (newCapacity < required) newCapacity = required;
        vec->data     = VECTOR_GROW(vec->data, oldCapacity, newCapacity, vec->elementSize);
        vec->capacity = newCapacity;
    }

    void* dest = (char*)vec->data + (vec->length * vec->elementSize);
    memcpy(dest, value, n * vec->elementSize);
    vec->length += n;
}

static inline void
vectorFree(Vector* vec)
{
    assert(vec);

    free(vec->data);
    vec->data        = NULL;
    vec->length      = 0;
    vec->capacity    = 0;
    vec->elementSize = 0;
}

static inline void*
vectorGet(const Vector* vec, size_t index)
{
    assert(vec);

    if (index >= vec->length) return NULL;
    return (char*)vec->data + (index * vec->elementSize);
}

static inline void
vectorGrowForAppend(Vector* vec)
{
    assert(vec);

    if (vec->length >= vec->capacity)
    {
        const size_t oldCapacity = vec->capacity;

        vec->capacity = CAPACITY_GROW(oldCapacity);
        vec->data     = VECTOR_GROW(vec->data, oldCapacity, vec->capacity, vec->elementSize);
    }

    vec->length++;
}

static inline Vector
vectorInit(size_t elementSize)
{
    assert(elementSize > 0);

    return (Vector){
        .data        = NULL,
        .length      = 0,
        .capacity    = 0,
        .elementSize = elementSize
    };
}

static inline bool
vectorIsEmpty(const Vector* vec)
{
    assert(vec);
    return vec->length == 0;
}

static inline void
vectorClear(Vector* vec)
{
    assert(vec);
    vec->length = 0;
}

static inline void
vectorSort(Vector* vec, int (*cmp)(const void*, const void*))
{
    assert(vec), assert(cmp);
    if (vec->length < 2) return;
    qsort(vec->data, vec->length, vec->elementSize, cmp);
}

static inline Vector
vectorCopy(const Vector* src)
{
    assert(src);

    Vector dest = vectorInit(src->elementSize);

    if (!vectorIsEmpty(src))
    {
        dest.capacity = src->length;
        dest.data     = reallocate(NULL, 0, src->elementSize * src->length);
        memcpy(dest.data, src->data, src->elementSize * src->length);
        dest.length = src->length;
    }

    return dest;
}

static inline size_t
vectorLastIndex(const Vector* vec)
{
    assert(vec), assert(vec->length > 0);
    return vec->length - 1;
}

static inline size_t
vectorLength(const Vector* vec)
{
    assert(vec);
    return vec->length;
}

static inline void
vectorRemove(Vector* vec, size_t index)
{
    assert(vec), assert(index < vec->length);

    if (index < vec->length - 1)
    {
        void*       dest  = (char*)vec->data + (index * vec->elementSize);
        const void* src   = (char*)vec->data + ((index + 1) * vec->elementSize);
        size_t      bytes = (vec->length - index - 1) * vec->elementSize;
        memmove(dest, src, bytes);
    }

    vec->length--;
}

static inline void
vectorSwap(Vector* vec, size_t a, size_t b)
{
    assert(vec), assert(a < vec->length), assert(b < vec->length);
    if (a == b) return;

#define SWAP_BUFFER_SIZE 512
    char  stackBuffer[SWAP_BUFFER_SIZE];
    char* temp = vec->elementSize <= SWAP_BUFFER_SIZE
                     ? stackBuffer
                     : ALLOCATE(char, vec->elementSize);

    void* elemA = vectorGet(vec, a);
    void* elemB = vectorGet(vec, b);

    memcpy(temp, elemA, vec->elementSize);
    memcpy(elemA, elemB, vec->elementSize);
    memcpy(elemB, temp, vec->elementSize);

    if (temp != stackBuffer) free(temp);
#undef SWAP_BUFFER_SIZE
}

static inline void
vectorIteratorInit(const Vector* vec, VectorIterator* iter)
{
    assert(vec), assert(iter);
    iter->vec   = vec;
    iter->index = (size_t)-1;
}

static inline bool
vectorIteratorHasNext(const VectorIterator* iter)
{
    assert(iter);
    return (iter->index + 1) < iter->vec->length;
}

static inline void
vectorIteratorAdvance(VectorIterator* iter)
{
    assert(iter), assert(vectorIteratorHasNext(iter));
    iter->index++;
}

static inline VectorIterator
vectorIteratorMake(const Vector* vec)
{
    assert(vec);

    return (VectorIterator){
        .vec   = vec,
        .index = (size_t)-1
    };
}

static inline VectorIterator
vectorIteratorMakeStartingAt(const Vector* vec, size_t start)
{
    assert(vec);

    return (VectorIterator){
        .vec   = vec,
        .index = start > 0 ? start - 1 : (size_t)-1
    };
}

static inline void*
vectorIteratorNext(VectorIterator* iter)
{
    assert(iter);
    if (!vectorIteratorHasNext(iter)) return NULL;
    iter->index++;
    return vectorGet(iter->vec, iter->index);
}

static inline void*
vectorIteratorPeek(VectorIterator* iter)
{
    assert(iter);
    VectorIterator tmp = *iter;
    return vectorIteratorNext(&tmp);
}

#endif /* WK_COMMON_VECTOR_H_ */
