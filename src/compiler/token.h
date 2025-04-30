#ifndef WK_COMPILER_TOKEN_H_
#define WK_COMPILER_TOKEN_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* common includes */
#include "common/key_chord.h"

typedef uint8_t TokenType;
enum
{
    /* single characters */
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_ELLIPSIS,

    /* preprocessor macros */

    /* string macros */
    TOKEN_INCLUDE,
    TOKEN_FOREGROUND_COLOR,
    TOKEN_FOREGROUND_KEY_COLOR,
    TOKEN_FOREGROUND_DELIMITER_COLOR,
    TOKEN_FOREGROUND_PREFIX_COLOR,
    TOKEN_FOREGROUND_CHORD_COLOR,
    TOKEN_BACKGROUND_COLOR,
    TOKEN_BORDER_COLOR,
    TOKEN_SHELL,
    TOKEN_FONT,
    TOKEN_IMPLICIT_ARRAY_KEYS,

    /* switch macros */
    TOKEN_DEBUG,
    TOKEN_SORT,
    TOKEN_TOP,
    TOKEN_BOTTOM,

    /* [-]integer macros */
    TOKEN_MENU_WIDTH,
    TOKEN_MENU_GAP,

    /* integer macros */
    TOKEN_MAX_COLUMNS,
    TOKEN_BORDER_WIDTH,
    TOKEN_WIDTH_PADDING,
    TOKEN_HEIGHT_PADDING,
    TOKEN_MENU_DELAY,

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
    TOKEN_IGNORE_SORT,
    TOKEN_UNHOOK,
    TOKEN_DEFLAG,
    TOKEN_NO_BEFORE,
    TOKEN_NO_AFTER,
    TOKEN_WRITE,
    TOKEN_EXECUTE,
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

    /* mods */
    TOKEN_MOD_CTRL,
    TOKEN_MOD_META,
    TOKEN_MOD_HYPER,
    TOKEN_MOD_SHIFT,

    /* keys */
    TOKEN_KEY,
    TOKEN_SPECIAL_KEY,

    /* control */
    TOKEN_NO_INTERP,
    TOKEN_EMPTY,
    TOKEN_ERROR,
    TOKEN_EOF,
    TOKEN_UNKNOWN,

    /* end */
    TOKEN_LAST,
};

typedef struct
{
    const char* start;
    const char* message;
    size_t length;
    size_t messageLength;
    uint32_t line;
    uint32_t column;
    TokenType type;
    SpecialKey special;
} Token;

void tokenCopy(const Token* from, Token* to);
void tokenErrorAt(const Token* token, const char* filepath, const char* fmt, ...);
bool tokenGetDouble(const Token* token, double* dest, bool debug);
bool tokenGetInt32(const Token* token, int32_t* dest, bool debug);
const char* tokenGetLiteral(const TokenType type);
bool tokenGetUint32(const Token* token, uint32_t* dest, bool debug);
void tokenInit(Token* token);
bool tokenIsHookType(const TokenType type);
bool tokenIsModType(const TokenType type);

#endif /* WK_COMPILER_TOKEN_H_ */
