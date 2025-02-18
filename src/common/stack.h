#ifndef WK_COMMON_STACK_H_
#define WK_COMMON_STACK_H_

#include <assert.h>

#include "array.h"

#define STACK_INIT(type) stackInit(sizeof(type))
#define STACK_PEEK(stack, type) ((type*)stackPeek(stack))
#define STACK_ITER_NEXT(iter, type) ((type*)stackIteratorNext(iter))

typedef Array Stack;
typedef ArrayIterator StackIterator;

static inline Stack
stackInit(size_t elementSize)
{
    return arrayInit(elementSize);
}

static inline void
stackPush(Stack* stack, const void* value)
{
    arrayAppend(stack, value);
}

static inline void*
stackPeek(const Stack* stack)
{
    assert(stack);

    if (stack->length == 0) return NULL;
    return arrayGet(stack, stack->length - 1);
}

static inline void
stackPop(Stack* stack)
{
    assert(stack), assert(stack->length > 0);

    stack->length--;
}

static inline void
stackFree(Stack* stack)
{
    arrayFree(stack);
}

static inline void
stackIteratorInit(const Stack* stack, StackIterator* iter)
{
    arrayIteratorInit(stack, iter);
}

static inline bool
stackIteratorHasNext(const StackIterator* iter)
{
    return arrayIteratorHasNext(iter);
}

static inline void*
stackIteratorNext(StackIterator* iter)
{
    return arrayIteratorNext(iter);
}

#endif /* WK_COMMON_STACK_H_ */
