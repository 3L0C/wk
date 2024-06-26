#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* common includes */
#include "common/debug.h"
#include "common/memory.h"

/* local includes */
#include "token.h"

void
copyToken(const Token* from, Token* to)
{
    assert(from), assert(to);

    to->type = from->type;
    to->start = from->start;
    to->length = from->length;
    to->line = from->line;
    to->column = from->column;
    to->message = from->message;
    to->messageLength = from->messageLength;
}

void
copyTokenArray(const TokenArray* from, TokenArray* to)
{
    assert(from), assert(to);

    if (from->count == 0)
    {
        initTokenArray(to);
        return;
    }
    to->tokens = ALLOCATE(Token, from->capacity);
    to->count = from->count;
    to->capacity = from->capacity;
    for (size_t i = 0; i < from->count; i++)
    {
        copyToken(&from->tokens[i], &to->tokens[i]);
    }
}

void
errorAtToken(const Token* token, const char* filepath, const char* fmt, ...)
{
    assert(token), assert(filepath), assert(fmt);

    fprintf(stderr, "%s:%u:%u: error", filepath, token->line, token->column);

    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end: ");
    }
    else if (token->type == TOKEN_ERROR)
    {
        fprintf(
            stderr,
            " at line %u: '%.*s': ",
            token->line, (int)token->length, token->start
        );
    }
    else
    {
        fprintf(stderr, " at '%.*s': ", (int)token->length, token->start);
    }

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr);
}

bool
getDoubleFromToken(const Token* token, double* dest, bool debug)
{
    assert(token), assert(dest);

    char* endptr;
    int oldErrno = errno;
    errno = 0;
    double value = strtod(token->start, &endptr);
    *dest = 0;

    if (errno == ERANGE)
    {
        debugMsg(
            debug,
            "Tried to parse string as double, but it was out of range: '%.*s'",
            token->length, token->start
        );
        errno = oldErrno;
        return false;
    }
    else if ((size_t)(endptr - token->start) != token->length)
    {
        debugMsg(
            debug,
            "Tried to parse string as double, but it was parsed differently than expected.\n\t"
            "String to be parsed:    '%.*s'\n\t"
            "String actually parsed: '%.*s'",
            token->length, token->start,
            (int)(endptr - token->start), token->start
        );
        return false;
    }

    *dest = (int32_t)value;
    return true;
}

bool
getInt32FromToken(const Token* token, int32_t* dest, bool debug)
{
    assert(token), assert(dest);

    char* endptr;
    int oldErrno = errno;
    errno = 0;
    intmax_t value = strtoimax(token->start, &endptr, 10);
    *dest = 0;

    if (errno == ERANGE)
    {
        debugMsg(
            debug,
            "Tried to parse string as uintmax_t, but it was out of range: '%.*s'",
            token->length, token->start
        );
        errno = oldErrno;
        return false;
    }
    else if (value > INT32_MAX)
    {
        debugMsg(
            debug,
            "Tried to parse string as int32_t, but it was too large: '%.*s'",
            token->length, token->start
        );
        return false;
    }
    else if ((size_t)(endptr - token->start) != token->length)
    {
        debugMsg(
            debug,
            "Tried to parse string as int32_t, but it was parsed differently than expected.\n\t"
            "String to be parsed:    '%.*s'\n\t"
            "String actually parsed: '%.*s'",
            token->length, token->start,
            (int)(endptr - token->start), token->start
        );
        return false;
    }

    *dest = (int32_t)value;
    return true;
}

bool
getUint32FromToken(const Token* token, uint32_t* dest, bool debug)
{
    assert(token), assert(dest);

    char* endptr;
    int oldErrno = errno;
    errno = 0;
    uintmax_t value = strtoumax(token->start, &endptr, 10);
    *dest = 0;

    if (errno == ERANGE)
    {
        debugMsg(
            debug,
            "Tried to parse string as uintmax_t, but it was out of range: '%.*s'",
            token->length, token->start
        );
        errno = oldErrno;
        return false;
    }
    else if (value > UINT32_MAX)
    {
        debugMsg(
            debug,
            "Tried to parse string as uint32_t, but it was too large: '%.*s'",
            token->length, token->start
        );
        return false;
    }
    else if ((size_t)(endptr - token->start) != token->length)
    {
        debugMsg(
            debug,
            "Tried to parse string as uint32_t, but it was parsed differently than expected.\n\t"
            "String to be parsed:    '%.*s'\n\t"
            "String actually parsed: '%.*s'",
            token->length, token->start,
            (int)(endptr - token->start), token->start
        );
        return false;
    }

    *dest = (uint32_t)value;
    return true;
}

const char*
getTokenLiteral(const TokenType type)
{
    switch (type)
    {
    /* single characters */
    case TOKEN_LEFT_BRACKET: return "TOKEN_LEFT_BRACKET";
    case TOKEN_RIGHT_BRACKET: return "TOKEN_RIGHT_BRACKET";
    case TOKEN_LEFT_BRACE: return "TOKEN_LEFT_BRACE";
    case TOKEN_RIGHT_BRACE: return "TOKEN_RIGHT_BRACE";
    case TOKEN_LEFT_PAREN: return "TOKEN_LEFT_PAREN";
    case TOKEN_RIGHT_PAREN: return "TOKEN_RIGHT_PAREN";
    case TOKEN_ELLIPSIS: return "TOKEN_ELLIPSIS";

    /* preprocessor macros */

    /* string macros */
    case TOKEN_INCLUDE: return "TOKEN_INCLUDE";
    case TOKEN_FOREGROUND_COLOR: return "TOKEN_FOREGROUND_COLOR";
    case TOKEN_FOREGROUND_KEY_COLOR: return "TOKEN_FOREGROUND_KEY_COLOR";
    case TOKEN_FOREGROUND_DELIMITER_COLOR: return "TOKEN_FOREGROUND_DELIMITER_COLOR";
    case TOKEN_FOREGROUND_PREFIX_COLOR: return "TOKEN_FOREGROUND_PREFIX_COLOR";
    case TOKEN_FOREGROUND_CHORD_COLOR: return "TOKEN_FOREGROUND_CHORD_COLOR";
    case TOKEN_BACKGROUND_COLOR: return "TOKEN_BACKGROUND_COLOR";
    case TOKEN_BORDER_COLOR: return "TOKEN_BORDER_COLOR";
    case TOKEN_SHELL: return "TOKEN_SHELL";
    case TOKEN_FONT: return "TOKEN_FONT";
    case TOKEN_IMPLICIT_ARRAY_KEYS: return "TOKEN_IMPLICIT_ARRAY_KEYS";

    /* switch macros */
    case TOKEN_DEBUG: return "TOKEN_DEBUG";
    case TOKEN_SORT: return "TOKEN_SORT";
    case TOKEN_TOP: return "TOKEN_TOP";
    case TOKEN_BOTTOM: return "TOKEN_BOTTOM";

    /* [-]integer macros */
    case TOKEN_MENU_WIDTH: return "TOKEN_MENU_WIDTH";
    case TOKEN_MENU_GAP: return "TOKEN_MENU_GAP";

    /* integer macros */
    case TOKEN_MAX_COLUMNS: return "TOKEN_MAX_COLUMNS";
    case TOKEN_BORDER_WIDTH: return "TOKEN_BORDER_WIDTH";
    case TOKEN_WIDTH_PADDING: return "TOKEN_WIDTH_PADDING";
    case TOKEN_HEIGHT_PADDING: return "TOKEN_HEIGHT_PADDING";
    case TOKEN_MENU_DELAY: return "TOKEN_MENU_DELAY";

    /* number macros */
    case TOKEN_BORDER_RADIUS: return "TOKEN_BORDER_RADIUS";

    /* identifiers */
    case TOKEN_THIS_KEY: return "TOKEN_THIS_KEY";
    case TOKEN_INDEX: return "TOKEN_INDEX";
    case TOKEN_INDEX_ONE: return "TOKEN_INDEX_ONE";
    case TOKEN_THIS_DESC: return "TOKEN_THIS_DESC";
    case TOKEN_THIS_DESC_UPPER_FIRST: return "TOKEN_THIS_DESC_UPPER_FIRST";
    case TOKEN_THIS_DESC_LOWER_FIRST: return "TOKEN_THIS_DESC_LOWER_FIRST";
    case TOKEN_THIS_DESC_UPPER_ALL: return "TOKEN_THIS_DESC_UPPER_ALL";
    case TOKEN_THIS_DESC_LOWER_ALL: return "TOKEN_THIS_DESC_LOWER_ALL";

    /* hooks */
    case TOKEN_BEFORE: return "TOKEN_BEFORE";
    case TOKEN_AFTER: return "TOKEN_AFTER";
    case TOKEN_SYNC_BEFORE: return "TOKEN_SYNC_BEFORE";
    case TOKEN_SYNC_AFTER: return "TOKEN_SYNC_AFTER";

    /* flags */
    case TOKEN_KEEP: return "TOKEN_KEEP";
    case TOKEN_CLOSE: return "TOKEN_CLOSE";
    case TOKEN_INHERIT: return "TOKEN_INHERIT";
    case TOKEN_IGNORE: return "TOKEN_IGNORE";
    case TOKEN_IGNORE_SORT: return "TOKEN_IGNORE_SORT";
    case TOKEN_UNHOOK: return "TOKEN_UNHOOK";
    case TOKEN_DEFLAG: return "TOKEN_DEFLAG";
    case TOKEN_NO_BEFORE: return "TOKEN_NO_BEFORE";
    case TOKEN_NO_AFTER: return "TOKEN_NO_AFTER";
    case TOKEN_WRITE: return "TOKEN_WRITE";
    case TOKEN_SYNC_CMD: return "TOKEN_SYNC_CMD";

    /* literals */
    case TOKEN_COMMAND: return "TOKEN_COMMAND";
    case TOKEN_DESCRIPTION: return "TOKEN_DESCRIPTION";
    case TOKEN_DOUBLE: return "TOKEN_DOUBLE";
    case TOKEN_INTEGER: return "TOKEN_INTEGER";
    case TOKEN_UNSIGNED_INTEGER: return "TOKEN_UNSIGNED_INTEGER";

    /* interpolations */
    case TOKEN_COMM_INTERP: return "TOKEN_COMM_INTERP";
    case TOKEN_DESC_INTERP: return "TOKEN_DESC_INTERP";

    /* keys */
    case TOKEN_KEY: return "TOKEN_KEY";

    /* mods */
    case TOKEN_MOD_CTRL: return "TOKEN_MOD_CTRL";
    case TOKEN_MOD_ALT: return "TOKEN_MOD_ALT";
    case TOKEN_MOD_HYPER: return "TOKEN_MOD_HYPER";
    case TOKEN_MOD_SHIFT: return "TOKEN_MOD_SHIFT";

    /* special keys */
    case TOKEN_SPECIAL_KEY: return "TOKEN_SPECIAL_KEY";

    /* control */
    case TOKEN_NO_INTERP: return "TOKEN_NO_INTERP";
    case TOKEN_ERROR: return "TOKEN_ERROR";
    case TOKEN_EOF: return "TOKEN_EOF";
    case TOKEN_EMPTY: return "TOKEN_EMPTY";
    }

    return "TOKEN_UNKNOWN";
}

void
initToken(Token* token)
{
    assert(token);

    token->type = TOKEN_EMPTY;
    token->start = NULL;
    token->length = 0;
    token->line = 0;
    token->column = 0;
    token->message = NULL;
    token->messageLength = 0;
}

void
initTokenArray(TokenArray* tokens)
{
    assert(tokens);

    tokens->tokens = NULL;
    tokens->count = 0;
    tokens->capacity = 0;
}

void
freeTokenArray(TokenArray* tokens)
{
    assert(tokens);

    FREE_ARRAY(Token, tokens->tokens, tokens->capacity);
    tokens->tokens = NULL;
    tokens->count = 0;
    tokens->capacity = 0;
}

bool
isTokenHookType(const TokenType type)
{
    return (
        type == TOKEN_BEFORE || type == TOKEN_AFTER ||
        type == TOKEN_SYNC_BEFORE || type == TOKEN_SYNC_AFTER
    );
}

bool
isTokenModType(const TokenType type)
{
    return (
        type == TOKEN_MOD_CTRL || type == TOKEN_MOD_ALT ||
        type == TOKEN_MOD_HYPER || type == TOKEN_MOD_SHIFT
    );
}

void
writeTokenArray(TokenArray* tokens, Token* token)
{
    assert(tokens), assert(token);

    if (tokens->count == tokens->capacity)
    {
        size_t oldCapacity = tokens->capacity;
        tokens->capacity = GROW_CAPACITY(oldCapacity);
        tokens->tokens = GROW_ARRAY(
            Token, tokens->tokens, oldCapacity, tokens->capacity
        );
    }

    copyToken(token, &tokens->tokens[tokens->count]);
    tokens->count++;
}
