#ifndef WK_COMPILER_TOKEN_H_
#define WK_COMPILER_TOKEN_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

    /* string macros */
    TOKEN_INCLUDE,
    TOKEN_FOREGROUND_COLOR,
    TOKEN_BACKGROUND_COLOR,
    TOKEN_BORDER_COLOR,
    TOKEN_SHELL,
    TOKEN_FONT,

    /* switch macros */
    TOKEN_DEBUG,
    TOKEN_SORT,
    TOKEN_TOP,
    TOKEN_BOTTOM,

    /* [-]digit macros */
    TOKEN_WINDOW_WIDTH,
    TOKEN_WINDOW_GAP,

    /* digit macros */
    TOKEN_MAX_COLUMNS,
    TOKEN_BORDER_WIDTH,
    TOKEN_WIDTH_PADDING,
    TOKEN_HEIGHT_PADDING,

    /* number macros */
    TOKEN_BORDER_RADIUS,

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
    TOKEN_DOUBLE,
    TOKEN_INTEGER,
    TOKEN_UNSIGNED_INTEGER,

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
    TOKEN_NO_INTERP,
    TOKEN_EMPTY,
    TOKEN_ERROR,
    TOKEN_EOF,
} TokenType;

typedef struct
{
    TokenType type;
    const char* start;
    size_t length;
    uint32_t line;
    uint32_t column;
    const char* message;
    size_t messageLength;
} Token;

typedef struct
{
    Token* tokens;
    size_t count;
    size_t capacity;
} TokenArray;

void copyToken(const Token* from, Token* to);
void copyTokenArray(const TokenArray* from, TokenArray* to);
void errorAtToken(const Token* token, const char* filepath, const char* fmt, ...);
bool getDoubleFromToken(const Token* token, double* dest, bool debug);
bool getInt32FromToken(const Token* token, int32_t* dest, bool debug);
bool getUint32FromToken(const Token* token, uint32_t* dest, bool debug);
const char* getTokenRepr(const TokenType type);
void initToken(Token* token);
void initTokenArray(TokenArray* tokens);
bool isTokenHookType(const TokenType type);
bool isTokenModType(const TokenType type);
void freeTokenArray(TokenArray* tokens);
void writeTokenArray(TokenArray* tokens, Token* token);

#endif /* WK_COMPILER_TOKEN_H_ */
