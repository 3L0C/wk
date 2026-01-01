#ifndef WK_COMPILER_EXPECT_H_
#define WK_COMPILER_EXPECT_H_

#include <stddef.h>
#include <stdint.h>

/* local includes */
#include "token.h"

typedef uint32_t Expectation;
enum
{
    EXPECT_NONE      = 0,
    EXPECT_MOD       = 1 << 0,
    EXPECT_KEY       = 1 << 1,
    EXPECT_DESC      = 1 << 2,
    EXPECT_HOOK      = 1 << 3,
    EXPECT_FLAG      = 1 << 4,
    EXPECT_COMMAND   = 1 << 5,
    EXPECT_META      = 1 << 6,
    EXPECT_LBRACE    = 1 << 7,
    EXPECT_RBRACE    = 1 << 8,
    EXPECT_LBRACKET  = 1 << 9,
    EXPECT_RBRACKET  = 1 << 10,
    EXPECT_LPAREN    = 1 << 11,
    EXPECT_RPAREN    = 1 << 12,
    EXPECT_ELLIPSIS  = 1 << 13,
    EXPECT_LESS_THAN = 1 << 14,
    EXPECT_INTERP    = 1 << 15,
    EXPECT_EOF       = 1 << 16,
};

enum
{
    EXPECT_KEY_START = EXPECT_MOD |
                       EXPECT_KEY |
                       EXPECT_ELLIPSIS |
                       EXPECT_LESS_THAN |
                       EXPECT_LBRACKET,

    EXPECT_AFTER_DESC = EXPECT_HOOK |
                        EXPECT_FLAG |
                        EXPECT_COMMAND |
                        EXPECT_META |
                        EXPECT_LBRACE,

    EXPECT_AFTER_HOOK = EXPECT_HOOK |
                        EXPECT_FLAG |
                        EXPECT_COMMAND |
                        EXPECT_META |
                        EXPECT_LBRACE,

    EXPECT_AFTER_FLAG = EXPECT_HOOK |
                        EXPECT_FLAG |
                        EXPECT_COMMAND |
                        EXPECT_META |
                        EXPECT_LBRACE,
};

const char* expectationToString(Expectation e, char* buffer, size_t size);
Expectation tokenToExpectation(TokenType type);

#endif /* WK_COMPILER_EXPECT_H_ */
