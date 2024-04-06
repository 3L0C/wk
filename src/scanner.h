#ifndef WK_SCANNER_H_
#define WK_SCANNER_H_

#include "lib/common.h"

typedef enum
{
    /* single characters */
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,

    /* keywords */
    TOKEN_INCLUDE,
    TOKEN_INDEX,
    TOKEN_INDEX_ONE,
    TOKEN_THIS_KEY,
    TOKEN_THIS_DESC,
    TOKEN_BEFORE,
    TOKEN_AFTER,
    TOKEN_KEEP,
    TOKEN_CLOSE,
    TOKEN_INHERIT,
    TOKEN_UNHOOK,
    TOKEN_NO_BEFORE,
    TOKEN_NO_AFTER,
    TOKEN_WRITE,
    TOKEN_SYNC_CMD,
    TOKEN_ASYNC_BEFORE,
    TOKEN_SYNC_AFTER,

    /* literals */
    TOKEN_COMMAND,
    TOKEN_COMM_INTERP,
    TOKEN_DESCRIPTION,
    TOKEN_DESC_INTERP,

    /* keys */
    TOKEN_KEY,

    /* mods */
    TOKEN_MOD_CTRL,
    TOKEN_MOD_ALT,
    TOKEN_MOD_HYPER,
    TOKEN_MOD_SHIFT,

    /* specials */
    TOKEN_SPECIAL_LEFT,
    TOKEN_SPECIAL_RIGHT,
    TOKEN_SPECIAL_UP,
    TOKEN_SPECIAL_DOWN,
    TOKEN_SPECIAL_TAB,
    TOKEN_SPECIAL_SPACE,
    TOKEN_SPECIAL_RETURN,
    TOKEN_SPECIAL_DELETE,
    TOKEN_SPECIAL_ESCAPE,
    TOKEN_SPECIAL_HOME,
    TOKEN_SPECIAL_PAGE_UP,
    TOKEN_SPECIAL_PAGE_DOWN,
    TOKEN_SPECIAL_END,
    TOKEN_SPECIAL_BEGIN,

    /* control */
    TOKEN_NO_INTERP, TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct
{
    const char* start;
    const char* current;
    size_t line;
    TokenType interpType;
    bool isInterpolation;
} Scanner;

typedef struct
{
    TokenType type;
    const char* start;
    size_t length;
    size_t line;
    const char* message;
    size_t messageLength;
} Token;

char advance(Scanner* scanner);
Token identifier(Scanner* scanner);
void initScanner(Scanner* scanner, const char* source);
bool isAtEnd(const Scanner* scanner);
bool match(Scanner* scanner, const char expected);
char peek(const Scanner* scanner);
char peekNext(const Scanner* scanner);
Token scanToken(Scanner* scanner);

#endif /* WK_SCANNER_H_ */
