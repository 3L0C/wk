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
#define ARRAY_GROW(ptr, oldCapacity, newCapacity, elementSize) \
    reallocate( \
        (ptr), (elementSize) * (oldCapacity), (elementSize) * (newCapacity) \
    )
#define CAPACITY_GROW(size) (size) == 0 ? 8 : (size) * 2
#define ARRAY_ITER_NEXT(iter, type) ((type*)arrayIteratorNext(iter))

typedef struct
{
    void* data;
    size_t length;
    size_t capacity;
    size_t elementSize;
} Array;

typedef struct
{
    const Array* arr;
    size_t index;
} ArrayIterator;

static inline void
arrayAppend(Array* arr, const void* value)
{
    assert(arr), assert(value);

    if (arr->length >= arr->capacity)
    {
        const size_t oldCapacity = arr->capacity;
        arr->capacity = CAPACITY_GROW(oldCapacity);
        arr->data = ARRAY_GROW(arr->data, oldCapacity, arr->capacity, arr->elementSize);
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
        while (newCapacity < required) newCapacity *= 2;
        arr->data = ARRAY_GROW(arr->data, oldCapacity, newCapacity, arr->elementSize);
        arr->capacity = newCapacity;
    }

    void* dest = (char*)arr->data + (arr->length * arr->elementSize);
    memcpy(dest, value, arr->elementSize);
    arr->length += n;
}

static inline void*
arrayGet(const Array* arr, size_t index)
{
    assert(arr);

    if (index >= arr->length) return NULL;
    return (char*)arr->data + (index * arr->elementSize);
}

static inline void
arrayFree(Array* arr)
{
    assert(arr);

    free(arr->data);
    arr->data = NULL;
    arr->length = 0;
    arr->capacity = 0;
    arr->elementSize = 0;
}

static inline Array
arrayInit(size_t elementSize)
{
    assert(elementSize > 0);

    return (Array){
        .data = NULL,
        .length = 0,
        .capacity = 0,
        .elementSize = elementSize
    };
}

static inline bool
arrayIsEmpty(const Array* arr)
{
    assert(arr);
    return arr->length == 0;
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
        void* dest = (char*)arr->data + (index * arr->elementSize);
        const void* src = (char*)arr->data + ((index + 1) * arr->elementSize);
        size_t bytes = (arr->length - index - 1) * arr->elementSize;
        memmove(dest, src, bytes);
    }

    arr->length--;
}

static inline void
arrayIteratorInit(const Array* arr, ArrayIterator* iter)
{
    assert(arr), assert(iter);
    iter->arr = arr;
    iter->index = 0;
}

static inline bool
arrayIteratorHasNext(const ArrayIterator* iter)
{
    assert(iter);
    return iter->index < iter->arr->length;
}

static inline ArrayIterator
arrayIteratorMake(const Array* arr)
{
    assert(arr);

    return (ArrayIterator){
        .arr = arr,
        .index = 0
    };
}

static inline void*
arrayIteratorNext(ArrayIterator* iter)
{
    assert(iter);
    if (!arrayIteratorHasNext(iter)) return NULL;
    void* value = arrayGet(iter->arr, iter->index);
    iter->index++;
    return value;
}

#endif /* WK_COMMON_ARRAY_H_ */
