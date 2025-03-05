#ifndef WK_COMMON_STACK_H_
#define WK_COMMON_STACK_H_

#include <assert.h>
#include <stddef.h>

#include "array.h"

#define STACK_INIT(type) ARRAY_INIT(type)
#define STACK_PEEK(stack, type) ((type*)stackPeek(stack))
#define STACK_AS_ARRAY(stack) ((Array)stack)

typedef Array Stack;

static inline Stack
stackInit(size_t elementSize)
{
    return arrayInit(elementSize);
}

static inline bool
stackIsEmpty(const Stack* stack)
{
    return arrayIsEmpty(stack);
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

#endif /* WK_COMMON_STACK_H_ */
