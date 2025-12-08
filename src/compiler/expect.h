#ifndef WK_COMPILER_EXPECT_H_
#define WK_COMPILER_EXPECT_H_

#include <stddef.h>
#include <stdint.h>

/* local includes */
#include "token.h"

typedef uint32_t Expectation;

#define EXPECT_NONE 0
#define EXPECT_MOD (1 << 0)
#define EXPECT_KEY (1 << 1)
#define EXPECT_DESC (1 << 2)
#define EXPECT_HOOK (1 << 3)
#define EXPECT_FLAG (1 << 4)
#define EXPECT_COMMAND (1 << 5)
#define EXPECT_META (1 << 6)
#define EXPECT_LBRACE (1 << 7)
#define EXPECT_RBRACE (1 << 8)
#define EXPECT_LBRACKET (1 << 9)
#define EXPECT_RBRACKET (1 << 10)
#define EXPECT_LPAREN (1 << 11)
#define EXPECT_RPAREN (1 << 12)
#define EXPECT_ELLIPSIS (1 << 13)
#define EXPECT_INTERP (1 << 14)
#define EXPECT_EOF (1 << 15)

#define EXPECT_KEY_START (EXPECT_MOD | EXPECT_KEY | EXPECT_ELLIPSIS | EXPECT_LBRACKET)
#define EXPECT_AFTER_DESC (EXPECT_HOOK | EXPECT_FLAG | EXPECT_COMMAND | EXPECT_META | EXPECT_LBRACE)
#define EXPECT_AFTER_HOOK (EXPECT_HOOK | EXPECT_FLAG | EXPECT_COMMAND | EXPECT_META | EXPECT_LBRACE)
#define EXPECT_AFTER_FLAG (EXPECT_HOOK | EXPECT_FLAG | EXPECT_COMMAND | EXPECT_META | EXPECT_LBRACE)

const char* expectationToString(Expectation e, char* buffer, size_t size);
Expectation tokenToExpectation(TokenType type);

#endif /* WK_COMPILER_EXPECT_H_ */
