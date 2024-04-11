#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

/* common includes */
#include "common/common.h"
#include "common/util.h"

/* local includes */
#include "scanner.h"

typedef enum
{
    CHAR_TYPE_STATUS_ERROR,
    CHAR_TYPE_STATUS_SUCCESS,
} CharTypeStatus;

typedef enum
{
    CHAR_TYPE_WHITESPACE,
    CHAR_TYPE_INTERP_END,
} CharType;

void
initScanner(Scanner* scanner, const char* source, const char* filePath)
{
    assert(scanner && source);

    scanner->head = source;
    scanner->start = source;
    scanner->current = source;
    scanner->filePath = filePath;
    scanner->line = 1;
    scanner->column = 0;
    scanner->interpType = TOKEN_NO_INTERP;
    scanner->isInterpolation = false;
    scanner->hadError = false;
}

static void
cloneScanner(Scanner* scanner, Scanner* clone)
{
    clone->head = scanner->head;
    clone->start = scanner->start;
    clone->current = scanner->current;
    clone->filePath = scanner->filePath;
    clone->line = scanner->line;
    clone->column = scanner->column;
    clone->interpType = scanner->interpType;
    clone->isInterpolation = scanner->isInterpolation;
}

static void
makeScannerCurrent(Scanner* scanner)
{
    scanner->start = scanner->current;
}

static bool
isAlpha(const char c)
{
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
            c == '+' || c == '-' || c == '_';
}

bool
isAtEnd(const Scanner* scanner)
{
    return *scanner->current == '\0';
}

static void
scannerUpdateLineAndColumn(Scanner* scanner, char c)
{
    switch (c)
    {
    case '\n': scanner->line++; scanner->column = 0; break;
    default: scanner->column++; break;
    }
}

static char
advanceScanner(Scanner* scanner)
{
    scannerUpdateLineAndColumn(scanner, *scanner->current++);
    return scanner->current[-1];
}

static char
peek(const Scanner* scanner)
{
    return *scanner->current;
}

static char
peekNext(const Scanner* scanner)
{
    if (isAtEnd(scanner)) return '\0';
    return scanner->current[1];
}

static bool
matchScanner(Scanner* scanner, const char expected)
{
    if (isAtEnd(scanner)) return false;
    if (*scanner->current != expected) return false;
    scanner->current++;
    return true;
}

static void
makeToken(const Scanner* scanner, Token* token, const TokenType type)
{
    assert(token);

    token->type = type;
    token->start = scanner->start;
    token->length = (int)(scanner->current - scanner->start);
    token->line = scanner->line;
    token->column = scanner->column;
    /* error */
    token->message = token->start;
    token->messageLength = token->length;
}

static void
errorToken(const Scanner* scanner, Token* token, const char* message)
{
    assert(token);

    token->type = TOKEN_ERROR;
    token->start = scanner->start;
    token->length = (int)(scanner->current - scanner->start);
    token->line = scanner->line;
    token->column = scanner->column;
    /* error */
    token->message = message;
    token->messageLength = (int)strlen(message);
}

static void
skipWhitespace(Scanner* scanner)
{
    while (true)
    {
        char c = peek(scanner);
        switch (c)
        {
        case '\n': /* FALLTHROUGH */
        case ' ' :
        case '\r':
        case '\t':
            advanceScanner(scanner);
            break;
        case '#':
            while (peek(scanner) != '\n' && !isAtEnd(scanner)) advanceScanner(scanner);
            break;
        default: makeScannerCurrent(scanner); return;
        }
    }
}

static bool
isKeyword(Scanner* scanner, int start, int length, const char* rest)
{
    return (
        scanner->current - scanner->start == start + length &&
        memcmp(scanner->start + start, rest, length) == 0
    );
}

static CharTypeStatus
seekToCharType(Scanner* scanner, CharType charType)
{
    switch (charType)
    {
    case CHAR_TYPE_WHITESPACE:
    {
        while (!isAtEnd(scanner) && !isspace(peek(scanner))) advanceScanner(scanner);
        break;
    }
    case CHAR_TYPE_INTERP_END:
    {
        while (!isAtEnd(scanner) && peek(scanner) != ')') advanceScanner(scanner);
        break;
    }
    default: return CHAR_TYPE_STATUS_ERROR;
    }
    return CHAR_TYPE_STATUS_SUCCESS;
}

static void
getHook(Scanner* scanner, Token* token)
{
    assert(scanner);

    /* Seek to end of keyword. Only fails if given invalid CharType parameter. */
    if (seekToCharType(scanner, CHAR_TYPE_WHITESPACE) == CHAR_TYPE_STATUS_ERROR)
    {
        return errorToken(scanner, token, "Internal error. Trying to seek to invalid char type.");
    }

    TokenType result = TOKEN_ERROR;

    /* Switch on start of keyword */
    switch (*scanner->start)
    {
    case 'a': if (isKeyword(scanner, 1, 4, "fter")) result = TOKEN_AFTER; break;
    case 'b': if (isKeyword(scanner, 1, 5, "efore")) result = TOKEN_BEFORE; break;
    case 's':
    {
        if (isKeyword(scanner, 1, 10, "ync-before")) result = TOKEN_SYNC_BEFORE;
        if (isKeyword(scanner, 1, 9,  "ync-after")) result = TOKEN_SYNC_AFTER;
        break;
    }
    default: break;
    }

    if (result == TOKEN_ERROR) return errorToken(scanner, token, "Got unexpected hook keyword.");
    return makeToken(scanner, token, result);
}

static void
getFlag(Scanner* scanner, Token* token)
{
    assert(scanner);

    /* Seek to end of keyword. Only fails if given invalid CharType parameter. */
    if (seekToCharType(scanner, CHAR_TYPE_WHITESPACE) == CHAR_TYPE_STATUS_ERROR)
    {
        return errorToken(scanner, token, "Internal error. Trying to seek to invalid char type.");
    }

    TokenType result = TOKEN_ERROR;

    /* Switch on start of keyword */
    switch (*scanner->start)
    {
    case 'k': if (isKeyword(scanner, 1, 3, "eep")) result = TOKEN_KEEP; break;
    case 'c': if (isKeyword(scanner, 1, 4, "lose")) result = TOKEN_CLOSE; break;
    case 'd': if (isKeyword(scanner, 1, 5, "eflag")) result = TOKEN_DEFLAG; break;
    case 'i':
    {
        if (isKeyword(scanner, 1, 6, "nherit")) result = TOKEN_INHERIT;
        else if (isKeyword(scanner, 1, 5, "gnore")) result = TOKEN_IGNORE;
        break;
    }
    case 'n':
    {
        if (isKeyword(scanner, 1, 8, "o-before")) result = TOKEN_NO_BEFORE;
        else if (isKeyword(scanner, 1, 7, "o-after")) result = TOKEN_NO_AFTER;
        break;
    }
    case 'u': if (isKeyword(scanner, 1, 5, "nhook")) result = TOKEN_UNHOOK; break;
    case 'w': if (isKeyword(scanner, 1, 4, "rite")) result = TOKEN_WRITE; break;
    case 's': if (isKeyword(scanner, 1, 11, "ync-command")) result = TOKEN_SYNC_CMD; break;
    default: break;
    }

    if (result == TOKEN_ERROR) return errorToken(scanner, token, "Got unexpected flag keyword.");
    return makeToken(scanner, token, result);
}

static void
getPreprocessorMacro(Scanner* scanner, Token* token)
{
    assert(scanner);

    /* Seek to end of keyword. Only fails if given invalid CharType parameter. */
    if (seekToCharType(scanner, CHAR_TYPE_WHITESPACE) == CHAR_TYPE_STATUS_ERROR)
    {
        return errorToken(scanner, token, "Internal error. Trying to seek to invalid char type.");
    }

    TokenType result = TOKEN_ERROR;

    /* Switch on start of keyword */
    switch (*scanner->start)
    {
    case 'i': if (isKeyword(scanner, 1, 6, "nclude")) result = TOKEN_INCLUDE; break;
    default: break;
    }

    if (result == TOKEN_ERROR) return errorToken(scanner, token, "Got unexpected preprocessor command.");
    return makeToken(scanner, token, result);
}

static void
getDescription(Scanner* scanner, Token* token)
{
    while (!isAtEnd(scanner))
    {
        switch (peek(scanner))
        {
        /* case '\'': /\* NOTE possible return *\/ */
        case '\"': /* NOTE possible return */
        {
            makeToken(scanner, token, TOKEN_DESCRIPTION);
            advanceScanner(scanner);
            return;
        }
        case '\n': scanner->line++; break;
        case '%': /* NOTE possible return */
        {
            if (peekNext(scanner) == '(')
            {
                scanner->isInterpolation = true;
                scanner->interpType = TOKEN_DESC_INTERP;
                makeToken(scanner, token, TOKEN_DESC_INTERP);
                advanceScanner(scanner); /* % */
                advanceScanner(scanner); /* ( */
                return;
            }
            break;
        }
        case '\\':
            /* if (peekNext(scanner) == '\'') */
            if (peekNext(scanner) == '\"')
            {
                advanceScanner(scanner);
            }
            break;
        }
        advanceScanner(scanner);
    }

    return errorToken(scanner, token, "Unterminated string");
}

static void
getCommand(Scanner* scanner, Token* token)
{
    static int32_t braces = 0;
    while (!isAtEnd(scanner))
    {
        char c = peek(scanner);
        switch (c)
        {
        case '\n': scanner->line++; break;
        case '{': braces++; break;
        case '}': /* NOTE POSSIBLE RETURN */
            if (peekNext(scanner) == '}' && braces == 0)
            {
                makeToken(scanner, token, TOKEN_COMMAND);
                advanceScanner(scanner); /* '}' */
                advanceScanner(scanner); /* '}' */
                return;
            }
            braces--;
            break;
        case '%': /* NOTE POSSIBLE RETURN */
            if (peekNext(scanner) == '(')
            {
                scanner->isInterpolation = true;
                scanner->interpType = TOKEN_COMM_INTERP;
                makeToken(scanner, token, TOKEN_COMM_INTERP);
                advanceScanner(scanner); /* % */
                advanceScanner(scanner); /* ( */
                return;
            }
            break;
        }
        advanceScanner(scanner);
    }

    return errorToken(scanner, token, "Expected '}}' but got end of file");
}

static TokenType
getMod(const char c)
{
    switch (c)
    {
    case 'C': return TOKEN_MOD_CTRL;
    case 'H': return TOKEN_MOD_HYPER;
    case 'M': return TOKEN_MOD_ALT;
    case 'S': return TOKEN_MOD_SHIFT;
    default: return TOKEN_ERROR;
    }
}

static TokenType
checkSpecial(Scanner* scanner, size_t start, size_t length, const char* rest, TokenType type)
{
    if (isKeyword(scanner, start, length, rest))
    {
        return type;
    }

    return TOKEN_ERROR;
}

static TokenType
specialType(Scanner* scanner)
{
    switch (*scanner->start)
    {
    case 'L': return checkSpecial(scanner, 1, 3, "eft", TOKEN_SPECIAL_LEFT);
    case 'R':
        if (isKeyword(scanner, 1, 4, "ight")) return TOKEN_SPECIAL_RIGHT;
        if (isKeyword(scanner, 1, 2, "ET")) return TOKEN_SPECIAL_RETURN;
        break;
    case 'U': return checkSpecial(scanner, 1, 1, "p", TOKEN_SPECIAL_UP);
    case 'D':
        if (isKeyword(scanner, 1, 3, "own")) return TOKEN_SPECIAL_DOWN;
        if (isKeyword(scanner, 1, 2, "EL")) return TOKEN_SPECIAL_DELETE;
        break;
    case 'T': return checkSpecial(scanner, 1, 2, "AB", TOKEN_SPECIAL_TAB);
    case 'S': return checkSpecial(scanner, 1, 2, "PC", TOKEN_SPECIAL_SPACE);
    case 'E':
        if (isKeyword(scanner, 1, 2, "SC")) return TOKEN_SPECIAL_ESCAPE;
        if (isKeyword(scanner, 1, 2, "nd")) return TOKEN_SPECIAL_END;
        break;
    case 'H': return checkSpecial(scanner, 1, 3, "ome", TOKEN_SPECIAL_HOME);
    case 'P':
        if (isKeyword(scanner, 1, 3, "gUp")) return TOKEN_SPECIAL_PAGE_UP;
        if (isKeyword(scanner, 1, 5, "gDown")) return TOKEN_SPECIAL_PAGE_DOWN;
        break;
    case 'B': return checkSpecial(scanner, 1, 4, "egin", TOKEN_SPECIAL_BEGIN);
    }

    return TOKEN_ERROR;
}

static void
getSpecialKey(Scanner* scanner, Token* token)
{
    while (isAlpha(peek(scanner))) advanceScanner(scanner);
    TokenType type = specialType(scanner);
    if (type == TOKEN_ERROR) return errorToken(scanner, token, "Invalid special key");
    return makeToken(scanner, token, type);
}

static void
getKey(Scanner* scanner, Token* token, char c)
{
    if (isUtf8MultiByteStartByte(c))
    {
        /* NOTE scanning multi byte character */
        while (isUtf8ContByte(peek(scanner)))
        {
            c = advanceScanner(scanner);
        }
    }
    else
    {
        /* NOTE possible special character */
        Scanner clone;
        cloneScanner(scanner, &clone);
        getSpecialKey(&clone, token);
        if (token->type != TOKEN_ERROR)
        {
            cloneScanner(&clone, scanner);
            return;
        }
    }

    return makeToken(scanner, token, TOKEN_KEY);
}

static void
checkInterpolation(Scanner* scanner, Token* token)
{
    assert(scanner);

    /* Seek to end of keyword. Only fails if given invalid CharType parameter. */
    if (seekToCharType(scanner, CHAR_TYPE_INTERP_END) == CHAR_TYPE_STATUS_ERROR)
    {
        return errorToken(scanner, token, "Internal error. Trying to seek to invalid char type.");
    }

    TokenType result = TOKEN_ERROR;

    /* Switch on start of keyword */
    switch (*scanner->start)
    {
    case 'k': if (isKeyword(scanner, 1, 2, "ey")) result = TOKEN_THIS_KEY; break;
    case 'i':
    {
        if (isKeyword(scanner, 1, 4, "ndex")) result = TOKEN_INDEX;
        else if (isKeyword(scanner, 1, 6, "ndex+1")) result = TOKEN_INDEX_ONE;
        break;
    }
    case 'd':
    {
        if      (isKeyword(scanner, 1, 3, "esc"))   result = TOKEN_THIS_DESC;
        else if (isKeyword(scanner, 1, 4, "esc^"))  result = TOKEN_THIS_DESC_UPPER_FIRST;
        else if (isKeyword(scanner, 1, 4, "esc,"))  result = TOKEN_THIS_DESC_LOWER_FIRST;
        else if (isKeyword(scanner, 1, 5, "esc^^")) result = TOKEN_THIS_DESC_UPPER_ALL;
        else if (isKeyword(scanner, 1, 5, "esc,,")) result = TOKEN_THIS_DESC_LOWER_ALL;

        /* If one of the description identifiers make sure not inside a description. */
        if (result != TOKEN_ERROR && scanner->interpType == TOKEN_DESC_INTERP)
        {
            return errorToken(scanner, token, "Cannot interpolate description within a description.");
        }

        break;
    }
    default: errorMsg("Unexpected inrepolation start: '%c'.", *scanner->start); break;
    }

    if (result == TOKEN_ERROR) return errorToken(scanner, token, "Got unexpected interpolation identifier.");
    return makeToken(scanner, token, result);
}

static void
getInterpolation(Scanner* scanner, Token* token)
{
    skipWhitespace(scanner);
    if (isAtEnd(scanner))
    {
        scanner->isInterpolation = false;
        return makeToken(scanner, token, TOKEN_EOF);
    }

    char c = advanceScanner(scanner);

    switch (c)
    {
    case ')':
        scanner->isInterpolation = false;
        makeScannerCurrent(scanner);
        if (scanner->interpType == TOKEN_DESC_INTERP)
        {
            return getDescription(scanner, token);
        }
        else if (scanner->interpType == TOKEN_COMM_INTERP)
        {
            return getCommand(scanner, token);
        }
        return errorToken(scanner, token, "Not sure how we got here...");
    default:
        return checkInterpolation(scanner, token);
    }
}

void
scanToken(Scanner* scanner, Token* result)
{
    assert(scanner && result);

    initToken(result);

    if (scanner->isInterpolation) return getInterpolation(scanner, result);

    skipWhitespace(scanner);
    if (isAtEnd(scanner)) return makeToken(scanner, result, TOKEN_EOF);

    char c = advanceScanner(scanner);

    switch (c)
    {
    /* single characters */
    case '[': return makeToken(scanner, result, TOKEN_LEFT_BRACKET);
    case ']': return makeToken(scanner, result, TOKEN_RIGHT_BRACKET);
    case '{': return makeToken(scanner, result, TOKEN_LEFT_BRACE);
    case '}': return makeToken(scanner, result, TOKEN_RIGHT_BRACE);
    case '(': return makeToken(scanner, result, TOKEN_LEFT_PAREN);
    case ')': return makeToken(scanner, result, TOKEN_RIGHT_PAREN);

    /* Hooks, flags, and preprocessor commands */
    case '^': makeScannerCurrent(scanner); return getHook(scanner, result);
    case '+': makeScannerCurrent(scanner); return getFlag(scanner, result);
    case ':': makeScannerCurrent(scanner); return getPreprocessorMacro(scanner, result);

    /* literals */
    case '\"':
        makeScannerCurrent(scanner);
        return getDescription(scanner, result);
    case '%':
        if (peek(scanner) != '{')
        {
            return getKey(scanner, result, c);
        }
        else if (matchScanner(scanner, '{') && !matchScanner(scanner, '{'))
        {
            return errorToken(
                scanner, result, "Expected '{' after '%{'. '{' must be escaped if it is meant to be a key."
            );
        }

        makeScannerCurrent(scanner);
        skipWhitespace(scanner);
        return getCommand(scanner, result);
    case '\\': makeScannerCurrent(scanner); return getKey(scanner, result, advanceScanner(scanner));

    /* keys */
    case 'C': /* FALLTHROUGH */
    case 'H':
    case 'M':
    case 'S':
        if (matchScanner(scanner, '-')) return makeToken(scanner, result, getMod(c));
        return getKey(scanner, result, c);
    default: return getKey(scanner, result, c);
    }

    return errorToken(scanner, result, "Unexpected character");
}
