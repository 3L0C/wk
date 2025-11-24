#ifndef WK_COMMON_ARRAY_H_
#define WK_COMMON_ARRAY_H_

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* local includes */
#include "memory.h"

#define ARRAY_INIT(type) arrayInit(sizeof(type))
#define ARRAY_GET(arr, type, index) ((type*)arrayGet((arr), (index)))
#define ARRAY_GET_LAST(arr, type) ARRAY_GET(arr, type, arrayLastIndex(arr))
#define ARRAY_AS(arr, type) ARRAY_GET(arr, type, 0)
#define ARRAY_GROW(ptr, oldCapacity, newCapacity, elementSize) \
    reallocate((ptr), (elementSize) * (oldCapacity), (elementSize) * (newCapacity))
#define ARRAY_APPEND_SLOT(arr, type) (arrayGrowForAppend(arr), ARRAY_GET_LAST(arr, type))
#define CAPACITY_GROW(size) (size) == 0 ? 8 : (size) * 2
#define ARRAY_ITER_NEXT(iter, type) ((type*)arrayIteratorNext(iter))
#define ARRAY_ITER_PEEK(iter, type) ((type*)arrayIteratorPeek(iter))
#define forRange(arr, type, var, startIdx, endIdx)                                        \
    for (ArrayIterator iter = arrayIteratorMakeStartingAt(arr, startIdx);                 \
         (iter.index == (size_t)(startIdx) - 1) ? (arrayIteratorHasNext(&iter)) : false;) \
        for (type* var = NULL;                                                            \
             (var = ARRAY_ITER_NEXT(&iter, type)) != NULL && iter.index < (size_t)(endIdx);)
#define forEachFrom(arr, type, var, startIdx) forRange(arr, type, var, startIdx, arrayLength(arr))
#define forEach(arr, type, var) forRange(arr, type, var, 0, arrayLength(arr))

typedef struct
{
    void*  data;
    size_t length;
    size_t capacity;
    size_t elementSize;
} Array;

typedef struct
{
    const Array* arr;
    size_t       index;
} ArrayIterator;

static inline void
arrayAppend(Array* arr, const void* value)
{
    assert(arr), assert(value);

    if (arr->length >= arr->capacity)
    {
        const size_t oldCapacity = arr->capacity;
        arr->capacity            = CAPACITY_GROW(oldCapacity);
        arr->data                = ARRAY_GROW(arr->data, oldCapacity, arr->capacity, arr->elementSize);
    }

    void* dest = (char*)arr->data + (arr->length * arr->elementSize);
    memcpy(dest, value, arr->elementSize);
    arr->length++;
}

static inline void
arrayAppendN(Array* arr, const void* value, size_t n)
{
    assert(arr), assert(value);
    if (n == 0) return;

    size_t required = arr->length + n;
    if (required > arr->capacity)
    {
        size_t oldCapacity = arr->capacity;
        size_t newCapacity = CAPACITY_GROW(oldCapacity);
        if (newCapacity < required) newCapacity = required;
        arr->data     = ARRAY_GROW(arr->data, oldCapacity, newCapacity, arr->elementSize);
        arr->capacity = newCapacity;
    }

    void* dest = (char*)arr->data + (arr->length * arr->elementSize);
    memcpy(dest, value, n * arr->elementSize);
    arr->length += n;
}

static inline void
arrayFree(Array* arr)
{
    assert(arr);

    free(arr->data);
    arr->data        = NULL;
    arr->length      = 0;
    arr->capacity    = 0;
    arr->elementSize = 0;
}

static inline void*
arrayGet(const Array* arr, size_t index)
{
    assert(arr);

    if (index >= arr->length) return NULL;
    return (char*)arr->data + (index * arr->elementSize);
}

static inline void
arrayGrowForAppend(Array* arr)
{
    assert(arr);

    if (arr->length >= arr->capacity)
    {
        const size_t oldCapacity = arr->capacity;
        arr->capacity            = CAPACITY_GROW(oldCapacity);
        arr->data                = ARRAY_GROW(arr->data, oldCapacity, arr->capacity, arr->elementSize);
    }

    arr->length++;
}

static inline Array
arrayInit(size_t elementSize)
{
    assert(elementSize > 0);

    return (Array){
        .data        = NULL,
        .length      = 0,
        .capacity    = 0,
        .elementSize = elementSize
    };
}

static inline bool
arrayIsEmpty(const Array* arr)
{
    assert(arr);
    return arr->length == 0;
}

static inline Array
arrayCopy(const Array* src)
{
    assert(src);

    Array dest = arrayInit(src->elementSize);

    if (!arrayIsEmpty(src))
    {
        dest.capacity = src->length;
        dest.data     = reallocate(NULL, 0, src->elementSize * src->length);
        memcpy(dest.data, src->data, src->elementSize * src->length);
        dest.length = src->length;
    }

    return dest;
}

static inline size_t
arrayLastIndex(const Array* arr)
{
    assert(arr), assert(arr->length > 0);
    return arr->length - 1;
}

static inline size_t
arrayLength(const Array* arr)
{
    assert(arr);
    return arr->length;
}

static inline void
arrayRemove(Array* arr, size_t index)
{
    assert(arr), assert(index < arr->length);

    if (index < arr->length - 1)
    {
        void*       dest  = (char*)arr->data + (index * arr->elementSize);
        const void* src   = (char*)arr->data + ((index + 1) * arr->elementSize);
        size_t      bytes = (arr->length - index - 1) * arr->elementSize;
        memmove(dest, src, bytes);
    }

    arr->length--;
}

static inline void
arraySwap(Array* arr, size_t a, size_t b)
{
    assert(arr), assert(a < arr->length), assert(b < arr->length);
    if (a == b) return;

#define SWAP_BUFFER_SIZE 512
    char  stackBuffer[SWAP_BUFFER_SIZE];
    char* temp = arr->elementSize <= SWAP_BUFFER_SIZE ? stackBuffer : ALLOCATE(char, arr->elementSize);

    void* elemA = arrayGet(arr, a);
    void* elemB = arrayGet(arr, b);

    memcpy(temp, elemA, arr->elementSize);
    memcpy(elemA, elemB, arr->elementSize);
    memcpy(elemB, temp, arr->elementSize);

    if (temp != stackBuffer) free(temp);
#undef SWAP_BUFFER_SIZE
}

static inline void
arrayIteratorInit(const Array* arr, ArrayIterator* iter)
{
    assert(arr), assert(iter);
    iter->arr   = arr;
    iter->index = (size_t)-1;
}

static inline bool
arrayIteratorHasNext(const ArrayIterator* iter)
{
    assert(iter);
    return (iter->index + 1) < iter->arr->length;
}

static inline void
arrayIteratorAdvance(ArrayIterator* iter)
{
    assert(iter), assert(arrayIteratorHasNext(iter));
    iter->index++;
}

static inline ArrayIterator
arrayIteratorMake(const Array* arr)
{
    assert(arr);

    return (ArrayIterator){
        .arr   = arr,
        .index = (size_t)-1
    };
}

static inline ArrayIterator
arrayIteratorMakeStartingAt(const Array* arr, size_t start)
{
    assert(arr);

    return (ArrayIterator){
        .arr   = arr,
        .index = start > 0 ? start - 1 : (size_t)-1
    };
}

static inline void*
arrayIteratorNext(ArrayIterator* iter)
{
    assert(iter);
    if (!arrayIteratorHasNext(iter)) return NULL;
    iter->index++;
    return arrayGet(iter->arr, iter->index);
}

static inline void*
arrayIteratorPeek(ArrayIterator* iter)
{
    assert(iter);
    ArrayIterator tmp = *iter;
    return arrayIteratorNext(&tmp);
}

#endif /* WK_COMMON_ARRAY_H_ */
