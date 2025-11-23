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
#include "common/key_chord.h"

/* local includes */
#include "token.h"

typedef char* TokenTable;

static const TokenTable tokenTable[TOKEN_LAST] = {
    /* single characters */
    [TOKEN_LEFT_BRACKET]  = "TOKEN_LEFT_BRACKET",
    [TOKEN_RIGHT_BRACKET] = "TOKEN_RIGHT_BRACKET",
    [TOKEN_LEFT_BRACE]    = "TOKEN_LEFT_BRACE",
    [TOKEN_RIGHT_BRACE]   = "TOKEN_RIGHT_BRACE",
    [TOKEN_LEFT_PAREN]    = "TOKEN_LEFT_PAREN",
    [TOKEN_RIGHT_PAREN]   = "TOKEN_RIGHT_PAREN",
    [TOKEN_ELLIPSIS]      = "TOKEN_ELLIPSIS",

    /* preprocessor macros */

    /* string macros */
    [TOKEN_INCLUDE]                    = "TOKEN_INCLUDE",
    [TOKEN_FOREGROUND_COLOR]           = "TOKEN_FOREGROUND_COLOR",
    [TOKEN_FOREGROUND_KEY_COLOR]       = "TOKEN_FOREGROUND_KEY_COLOR",
    [TOKEN_FOREGROUND_DELIMITER_COLOR] = "TOKEN_FOREGROUND_DELIMITER_COLOR",
    [TOKEN_FOREGROUND_PREFIX_COLOR]    = "TOKEN_FOREGROUND_PREFIX_COLOR",
    [TOKEN_FOREGROUND_CHORD_COLOR]     = "TOKEN_FOREGROUND_CHORD_COLOR",
    [TOKEN_FOREGROUND_TITLE_COLOR]     = "TOKEN_FOREGROUND_TITLE_COLOR",
    [TOKEN_BACKGROUND_COLOR]           = "TOKEN_BACKGROUND_COLOR",
    [TOKEN_BORDER_COLOR]               = "TOKEN_BORDER_COLOR",
    [TOKEN_SHELL]                      = "TOKEN_SHELL",
    [TOKEN_FONT]                       = "TOKEN_FONT",
    [TOKEN_IMPLICIT_ARRAY_KEYS]        = "TOKEN_IMPLICIT_ARRAY_KEYS",
    [TOKEN_VAR]                        = "TOKEN_VAR",
    [TOKEN_WRAP_CMD]                   = "TOKEN_WRAP_CMD",
    [TOKEN_DELIMITER]                  = "TOKEN_DELIMITER",
    [TOKEN_MENU_TITLE]                 = "TOKEN_MENU_TITLE",
    [TOKEN_MENU_TITLE_FONT]            = "TOKEN_MENU_TITLE_FONT",

    /* switch macros */
    [TOKEN_DEBUG]  = "TOKEN_DEBUG",
    [TOKEN_SORT]   = "TOKEN_SORT",
    [TOKEN_TOP]    = "TOKEN_TOP",
    [TOKEN_BOTTOM] = "TOKEN_BOTTOM",

    /* [-]integer macros */
    [TOKEN_MENU_WIDTH] = "TOKEN_MENU_WIDTH",
    [TOKEN_MENU_GAP]   = "TOKEN_MENU_GAP",

    /* integer macros */
    [TOKEN_MAX_COLUMNS]    = "TOKEN_MAX_COLUMNS",
    [TOKEN_BORDER_WIDTH]   = "TOKEN_BORDER_WIDTH",
    [TOKEN_WIDTH_PADDING]  = "TOKEN_WIDTH_PADDING",
    [TOKEN_HEIGHT_PADDING] = "TOKEN_HEIGHT_PADDING",
    [TOKEN_TABLE_PADDING]  = "TOKEN_TABLE_PADDING",
    [TOKEN_MENU_DELAY]     = "TOKEN_MENU_DELAY",
    [TOKEN_KEEP_DELAY]     = "TOKEN_KEEP_DELAY",

    /* number macros */
    [TOKEN_BORDER_RADIUS] = "TOKEN_BORDER_RADIUS",

    /* identifiers */
    [TOKEN_THIS_KEY]              = "TOKEN_THIS_KEY",
    [TOKEN_INDEX]                 = "TOKEN_INDEX",
    [TOKEN_INDEX_ONE]             = "TOKEN_INDEX_ONE",
    [TOKEN_THIS_DESC]             = "TOKEN_THIS_DESC",
    [TOKEN_THIS_DESC_UPPER_FIRST] = "TOKEN_THIS_DESC_UPPER_FIRST",
    [TOKEN_THIS_DESC_LOWER_FIRST] = "TOKEN_THIS_DESC_LOWER_FIRST",
    [TOKEN_THIS_DESC_UPPER_ALL]   = "TOKEN_THIS_DESC_UPPER_ALL",
    [TOKEN_THIS_DESC_LOWER_ALL]   = "TOKEN_THIS_DESC_LOWER_ALL",
    [TOKEN_USER_VAR]              = "TOKEN_USER_VAR",

    /* hooks */
    [TOKEN_BEFORE]      = "TOKEN_BEFORE",
    [TOKEN_AFTER]       = "TOKEN_AFTER",
    [TOKEN_SYNC_BEFORE] = "TOKEN_SYNC_BEFORE",
    [TOKEN_SYNC_AFTER]  = "TOKEN_SYNC_AFTER",

    /* flags */
    [TOKEN_KEEP]        = "TOKEN_KEEP",
    [TOKEN_CLOSE]       = "TOKEN_CLOSE",
    [TOKEN_INHERIT]     = "TOKEN_INHERIT",
    [TOKEN_IGNORE]      = "TOKEN_IGNORE",
    [TOKEN_IGNORE_SORT] = "TOKEN_IGNORE_SORT",
    [TOKEN_UNHOOK]      = "TOKEN_UNHOOK",
    [TOKEN_DEFLAG]      = "TOKEN_DEFLAG",
    [TOKEN_NO_BEFORE]   = "TOKEN_NO_BEFORE",
    [TOKEN_NO_AFTER]    = "TOKEN_NO_AFTER",
    [TOKEN_WRITE]       = "TOKEN_WRITE",
    [TOKEN_EXECUTE]     = "TOKEN_EXECUTE",
    [TOKEN_SYNC_CMD]    = "TOKEN_SYNC_CMD",
    [TOKEN_WRAP]        = "TOKEN_WRAP",
    [TOKEN_UNWRAP]      = "TOKEN_UNWRAP",
    [TOKEN_ENABLE_SORT] = "TOKEN_ENABLE_SORT",
    [TOKEN_TITLE]       = "TOKEN_TITLE",

    /* literals */
    [TOKEN_COMMAND]          = "TOKEN_COMMAND",
    [TOKEN_DESCRIPTION]      = "TOKEN_DESCRIPTION",
    [TOKEN_DOUBLE]           = "TOKEN_DOUBLE",
    [TOKEN_INTEGER]          = "TOKEN_INTEGER",
    [TOKEN_UNSIGNED_INTEGER] = "TOKEN_UNSIGNED_INTEGER",

    /* interpolations */
    [TOKEN_COMM_INTERP] = "TOKEN_COMM_INTERP",
    [TOKEN_DESC_INTERP] = "TOKEN_DESC_INTERP",

    /* keys */
    [TOKEN_KEY] = "TOKEN_KEY",

    /* mods */
    [TOKEN_MOD_CTRL]  = "TOKEN_MOD_CTRL",
    [TOKEN_MOD_META]  = "TOKEN_MOD_META",
    [TOKEN_MOD_HYPER] = "TOKEN_MOD_HYPER",
    [TOKEN_MOD_SHIFT] = "TOKEN_MOD_SHIFT",

    /* special keys */
    [TOKEN_SPECIAL_KEY] = "TOKEN_SPECIAL_KEY",

    /* control */
    [TOKEN_NO_INTERP] = "TOKEN_NO_INTERP",
    [TOKEN_ERROR]     = "TOKEN_ERROR",
    [TOKEN_EOF]       = "TOKEN_EOF",
    [TOKEN_EMPTY]     = "TOKEN_EMPTY",
    [TOKEN_UNKNOWN]   = "TOKEN_UNKNOWN",
};

void
tokenCopy(const Token* from, Token* to)
{
    assert(from), assert(to);

    to->start         = from->start;
    to->message       = from->message;
    to->length        = from->length;
    to->messageLength = from->messageLength;
    to->line          = from->line;
    to->column        = from->column;
    to->type          = from->type;
    to->special       = from->special;
}

void
tokenErrorAt(const Token* token, const char* filepath, const char* fmt, ...)
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
            token->line,
            (int)token->length,
            token->start);
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
tokenGetDouble(const Token* token, double* dest, bool debug)
{
    assert(token), assert(dest);

    char* endptr;
    int   oldErrno = errno;
    errno          = 0;
    double value   = strtod(token->start, &endptr);
    *dest          = 0;

    if (errno == ERANGE)
    {
        debugMsg(
            debug,
            "Tried to parse string as double, but it was out of range: '%.*s'",
            token->length,
            token->start);
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
            token->length,
            token->start,
            (int)(endptr - token->start),
            token->start);
        return false;
    }

    *dest = (int32_t)value;
    return true;
}

bool
tokenGetInt32(const Token* token, int32_t* dest, bool debug)
{
    assert(token), assert(dest);

    char* endptr;
    int   oldErrno = errno;
    errno          = 0;
    intmax_t value = strtoimax(token->start, &endptr, 10);
    *dest          = 0;

    if (errno == ERANGE)
    {
        debugMsg(
            debug,
            "Tried to parse string as uintmax_t, but it was out of range: '%.*s'",
            token->length,
            token->start);
        errno = oldErrno;
        return false;
    }
    else if (value > INT32_MAX)
    {
        debugMsg(
            debug,
            "Tried to parse string as int32_t, but it was too large: '%.*s'",
            token->length,
            token->start);
        return false;
    }
    else if ((size_t)(endptr - token->start) != token->length)
    {
        debugMsg(
            debug,
            "Tried to parse string as int32_t, but it was parsed differently than expected.\n\t"
            "String to be parsed:    '%.*s'\n\t"
            "String actually parsed: '%.*s'",
            token->length,
            token->start,
            (int)(endptr - token->start),
            token->start);
        return false;
    }

    *dest = (int32_t)value;
    return true;
}

const char*
tokenGetLiteral(const TokenType type)
{
    return tokenTable[type];
}

bool
tokenGetUint32(const Token* token, uint32_t* dest, bool debug)
{
    assert(token), assert(dest);

    char* endptr;
    int   oldErrno  = errno;
    errno           = 0;
    uintmax_t value = strtoumax(token->start, &endptr, 10);
    *dest           = 0;

    if (errno == ERANGE)
    {
        debugMsg(
            debug,
            "Tried to parse string as uintmax_t, but it was out of range: '%.*s'",
            token->length,
            token->start);
        errno = oldErrno;
        return false;
    }
    else if (value > UINT32_MAX)
    {
        debugMsg(
            debug,
            "Tried to parse string as uint32_t, but it was too large: '%.*s'",
            token->length,
            token->start);
        return false;
    }
    else if ((size_t)(endptr - token->start) != token->length)
    {
        debugMsg(
            debug,
            "Tried to parse string as uint32_t, but it was parsed differently than expected.\n\t"
            "String to be parsed:    '%.*s'\n\t"
            "String actually parsed: '%.*s'",
            token->length,
            token->start,
            (int)(endptr - token->start),
            token->start);
        return false;
    }

    *dest = (uint32_t)value;
    return true;
}

void
tokenInit(Token* token)
{
    assert(token);

    token->start         = NULL;
    token->message       = NULL;
    token->length        = 0;
    token->messageLength = 0;
    token->line          = 0;
    token->column        = 0;
    token->type          = TOKEN_EMPTY;
    token->special       = SPECIAL_KEY_NONE;
}

bool
tokenIsHookType(const TokenType type)
{
    return (
        type == TOKEN_BEFORE || type == TOKEN_AFTER ||
        type == TOKEN_SYNC_BEFORE || type == TOKEN_SYNC_AFTER);
}

bool
tokenIsModType(const TokenType type)
{
    return (
        type == TOKEN_MOD_CTRL || type == TOKEN_MOD_META ||
        type == TOKEN_MOD_HYPER || type == TOKEN_MOD_SHIFT);
}
