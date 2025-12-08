#ifndef WK_COMMON_STACK_H_
#define WK_COMMON_STACK_H_

#include <assert.h>
#include <stddef.h>

#include "vector.h"

#define STACK_INIT(type) VECTOR_INIT(type)
#define STACK_PEEK(stack, type) ((type*)stackPeek(stack))
#define STACK_AS_VECTOR(stack) ((Vector)stack)

typedef Vector Stack;

static inline void
stackFree(Stack* stack)
{
    vectorFree(stack);
}

static inline Stack
stackInit(size_t elementSize)
{
    return vectorInit(elementSize);
}

static inline bool
stackIsEmpty(const Stack* stack)
{
    return vectorIsEmpty(stack);
}

static inline void*
stackPeek(const Stack* stack)
{
    assert(stack);

    if (stack->length == 0) return NULL;
    return vectorGet(stack, stack->length - 1);
}

static inline void
stackPop(Stack* stack)
{
    assert(stack), assert(stack->length > 0);

    stack->length--;
}

static inline void
stackPush(Stack* stack, const void* value)
{
    vectorAppend(stack, value);
}

#endif /* WK_COMMON_STACK_H_ */
