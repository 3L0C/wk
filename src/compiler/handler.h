#ifndef WK_COMPILER_HANDLER_H_
#define WK_COMPILER_HANDLER_H_

#include <stdbool.h>

/* local includes */
#include "expect.h"
#include "token.h"

typedef struct Parser Parser;

typedef struct
{
    Expectation next;
    bool        error;
} HandleResult;

typedef HandleResult (*TokenHandler)(Parser*);

TokenHandler getHandler(TokenType type);
HandleResult handleCurrent(Parser* p);

static inline HandleResult
handleResultError(void)
{
    return (HandleResult){ .next = EXPECT_NONE, .error = true };
}

static inline HandleResult
handleResultOk(Expectation next)
{
    return (HandleResult){ .next = next, .error = false };
}

#endif /* WK_COMPILER_HANDLER_H_ */
