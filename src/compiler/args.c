#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

/* common includes */
#include "common/vector.h"

/* local includes */
#include "args.h"

void
argEnvAddArg(ArgEnvironment* env, Vector* tokens)
{
    assert(env), assert(tokens);

    Arg arg = { .tokens = *tokens, .used = false };
    vectorAppend(&env->args, &arg);
}

void
argEnvFree(ArgEnvironment* env)
{
    assert(env);

    vectorForEach(&env->args, Arg, arg)
    {
        vectorFree(&arg->tokens);
    }
    vectorFree(&env->args);
}

Vector*
argEnvGet(ArgEnvironment* env, size_t index)
{
    assert(env);

    Arg* arg = VECTOR_GET(&env->args, Arg, index);
    if (!arg) return NULL;

    arg->used = true;
    return &arg->tokens;
}

const Vector*
argEnvGetConst(const ArgEnvironment* env, size_t index)
{
    assert(env);

    const Arg* arg = VECTOR_GET(&env->args, Arg, index);
    return arg ? &arg->tokens : NULL;
}

void
argEnvInit(ArgEnvironment* env)
{
    assert(env);
    env->args = VECTOR_INIT(Arg);
}

bool
argEnvIsEmpty(const ArgEnvironment* env)
{
    assert(env);
    return vectorIsEmpty(&env->args);
}

Vector*
argEnvStackLookup(Stack* stack, size_t index)
{
    assert(stack);

    size_t depth = vectorLength(stack);
    for (size_t i = depth; i > 0; i--)
    {
        ArgEnvironment* env = VECTOR_GET(stack, ArgEnvironment, i - 1);
        Vector*         arg = argEnvGet(env, index);
        if (arg) return arg;
    }

    return NULL;
}

const Vector*
argEnvStackLookupConst(const Stack* stack, size_t index)
{
    assert(stack);

    size_t depth = vectorLength(stack);
    for (size_t i = depth; i > 0; i--)
    {
        const ArgEnvironment* env = VECTOR_GET(stack, ArgEnvironment, i - 1);
        const Vector*         arg = argEnvGetConst(env, index);
        if (arg) return arg;
    }

    return NULL;
}

void
argEnvWarnUnused(ArgEnvironment* env, Scanner* scanner)
{
    assert(env);

    vectorForEach(&env->args, Arg, arg)
    {
        if (!arg->used && !vectorIsEmpty(&arg->tokens))
        {
            Token* token = VECTOR_GET(&arg->tokens, Token, 0);
            scannerWarnAt(
                scanner,
                token,
                "Argument $%zu defined but never used.",
                iter.index);
        }
    }
}
