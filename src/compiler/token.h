#ifndef WK_COMPILER_TOKEN_H_
#define WK_COMPILER_TOKEN_H_

#include <stddef.h>

typedef enum
{
    /* single characters */
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,

    /* preprocessor macros */
    TOKEN_INCLUDE,

    /* identifiers */
    TOKEN_THIS_KEY,
    TOKEN_INDEX,
    TOKEN_INDEX_ONE,
    TOKEN_THIS_DESC,
    TOKEN_THIS_DESC_UPPER_FIRST,
    TOKEN_THIS_DESC_LOWER_FIRST,
    TOKEN_THIS_DESC_UPPER_ALL,
    TOKEN_THIS_DESC_LOWER_ALL,

    /* hooks */
    TOKEN_BEFORE,
    TOKEN_AFTER,
    TOKEN_SYNC_BEFORE,
    TOKEN_SYNC_AFTER,

    /* flags */
    TOKEN_KEEP,
    TOKEN_CLOSE,
    TOKEN_INHERIT,
    TOKEN_IGNORE,
    TOKEN_UNHOOK,
    TOKEN_DEFLAG,
    TOKEN_NO_BEFORE,
    TOKEN_NO_AFTER,
    TOKEN_WRITE,
    TOKEN_SYNC_CMD,

    /* literals */
    TOKEN_COMMAND,
    TOKEN_DESCRIPTION,

    /* interpolations */
    TOKEN_COMM_INTERP,
    TOKEN_DESC_INTERP,

    /* keys */
    TOKEN_KEY,

    /* mods */
    TOKEN_MOD_CTRL,
    TOKEN_MOD_ALT,
    TOKEN_MOD_HYPER,
    TOKEN_MOD_SHIFT,

    /* special keys */
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
    TOKEN_NO_INTERP, TOKEN_ERROR, TOKEN_EOF, TOKEN_EMPTY,
} TokenType;

typedef struct
{
    TokenType type;
    const char* start;
    size_t length;
    size_t line;
    size_t column;
    const char* message;
    size_t messageLength;
} Token;

typedef struct
{
    Token* tokens;
    size_t count;
    size_t capacity;
} TokenArray;

void copyToken(Token* from, Token* to);
void copyTokenArray(TokenArray* from, TokenArray* to);
void initToken(Token* token);
void initTokenArray(TokenArray* array);
void freeTokenArray(TokenArray* array);
void writeTokenArray(TokenArray* array, Token* token);

#endif /* WK_COMPILER_TOKEN_H_ */
