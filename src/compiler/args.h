#ifndef WK_COMPILER_ARGS_H_
#define WK_COMPILER_ARGS_H_

#include "common/stack.h"
#include "common/vector.h"
#include "scanner.h"

typedef struct
{
    Vector tokens;
    bool   used;
} Arg;

typedef struct
{
    Vector args;
} ArgEnvironment;

void          argEnvAddArg(ArgEnvironment* env, Vector* arg);
void          argEnvFree(ArgEnvironment* env);
Vector*       argEnvGet(ArgEnvironment* env, size_t index);
const Vector* argEnvGetConst(const ArgEnvironment* env, size_t index);
void          argEnvInit(ArgEnvironment* env);
bool          argEnvIsEmpty(const ArgEnvironment* env);
Vector*       argEnvStackLookup(Stack* stack, size_t index);
const Vector* argEnvStackLookupConst(const Stack* stack, size_t index);
void          argEnvWarnUnused(ArgEnvironment* env, Scanner* scanner);

#endif /* WK_COMPILER_ARGS_H_ */
