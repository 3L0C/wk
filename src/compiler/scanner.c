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
initScanner(Scanner* scanner, const char* source)
{
    assert(scanner && source);

    scanner->head = source;
    scanner->start = source;
    scanner->current = source;
    scanner->line = 1;
    scanner->interpType = TOKEN_NO_INTERP;
    scanner->isInterpolation = false;
}

static void
cloneScanner(Scanner* scanner, Scanner* clone)
{
    clone->head = scanner->head;
    clone->start = scanner->start;
    clone->current = scanner->current;
    clone->line = scanner->line;
    clone->interpType = scanner->interpType;
    clone->isInterpolation = scanner->isInterpolation;
}

void
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

char
advanceScanner(Scanner* scanner)
{
    scanner->current++;
    return scanner->current[-1];
}

char
peek(const Scanner* scanner)
{
    return *scanner->current;
}

char
peekNext(const Scanner* scanner)
{
    if (isAtEnd(scanner)) return '\0';
    return scanner->current[1];
}

bool
matchScanner(Scanner* scanner, const char expected)
{
    if (isAtEnd(scanner)) return false;
    if (*scanner->current != expected) return false;
    scanner->current++;
    return true;
}

static Token
makeToken(const Scanner* scanner, const TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner->start;
    token.length = (int)(scanner->current - scanner->start);
    token.line = scanner->line;
    /* error */
    token.message = token.start;
    token.messageLength = token.length;
    return token;
}

static Token
errorToken(const Scanner* scanner, const char* message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = scanner->start;
    token.length = (int)(scanner->current - scanner->start);
    token.line = scanner->line;
    /* error */
    token.message = message;
    token.messageLength = (int)strlen(message);
    return token;
}

void
skipWhitespace(Scanner* scanner)
{
    while (true)
    {
        char c = peek(scanner);
        switch (c)
        {
        case '\n': scanner->line++; /* FALLTHROUGH */
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

static Token
getHook(Scanner* scanner)
{
    assert(scanner);

    /* Seek to end of keyword. Only fails if given invalid CharType parameter. */
    if (seekToCharType(scanner, CHAR_TYPE_WHITESPACE) == CHAR_TYPE_STATUS_ERROR)
    {
        return errorToken(scanner, "Internal error. Trying to seek to invalid char type.");
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

    if (result == TOKEN_ERROR) return errorToken(scanner, "Got unexpected hook keyword.");
    return makeToken(scanner, result);
}

static Token
getFlag(Scanner* scanner)
{
    assert(scanner);

    /* Seek to end of keyword. Only fails if given invalid CharType parameter. */
    if (seekToCharType(scanner, CHAR_TYPE_WHITESPACE) == CHAR_TYPE_STATUS_ERROR)
    {
        return errorToken(scanner, "Internal error. Trying to seek to invalid char type.");
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

    if (result == TOKEN_ERROR) return errorToken(scanner, "Got unexpected flag keyword.");
    return makeToken(scanner, result);
}

Token
getPreprocessorCommand(Scanner* scanner)
{
    assert(scanner);

    /* Seek to end of keyword. Only fails if given invalid CharType parameter. */
    if (seekToCharType(scanner, CHAR_TYPE_WHITESPACE) == CHAR_TYPE_STATUS_ERROR)
    {
        return errorToken(scanner, "Internal error. Trying to seek to invalid char type.");
    }

    TokenType result = TOKEN_ERROR;

    /* Switch on start of keyword */
    switch (*scanner->start)
    {
    case 'i': if (isKeyword(scanner, 1, 6, "nclude")) result = TOKEN_INCLUDE; break;
    default: break;
    }

    if (result == TOKEN_ERROR) return errorToken(scanner, "Got unexpected preprocessor command.");
    return makeToken(scanner, result);
}

static Token
getDescription(Scanner* scanner)
{
    while (!isAtEnd(scanner))
    {
        switch (peek(scanner))
        {
        /* case '\'': /\* NOTE possible return *\/ */
        case '\"': /* NOTE possible return */
        {
            Token result = makeToken(scanner, TOKEN_DESCRIPTION);
            advanceScanner(scanner);
            return result;
        }
        case '\n': scanner->line++; break;
        case '%': /* NOTE possible return */
        {
            if (peekNext(scanner) == '(')
            {
                scanner->isInterpolation = true;
                scanner->interpType = TOKEN_DESC_INTERP;
                Token result = makeToken(scanner, TOKEN_DESC_INTERP);
                advanceScanner(scanner); /* % */
                advanceScanner(scanner); /* ( */
                return result;
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

    return errorToken(scanner, "Unterminated string");
}

static Token
getCommand(Scanner* scanner)
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
                Token result = makeToken(scanner, TOKEN_COMMAND);
                advanceScanner(scanner); /* '}' */
                advanceScanner(scanner); /* '}' */
                return result;
            }
            braces--;
            break;
        case '%': /* NOTE POSSIBLE RETURN */
            if (peekNext(scanner) == '(')
            {
                scanner->isInterpolation = true;
                scanner->interpType = TOKEN_COMM_INTERP;
                Token result = makeToken(scanner, TOKEN_COMM_INTERP);
                advanceScanner(scanner); /* % */
                advanceScanner(scanner); /* ( */
                return result;
            }
            break;
        }
        advanceScanner(scanner);
    }

    return errorToken(scanner, "Expected '}}' but got end of file");
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

static Token
getSpecialKey(Scanner* scanner)
{
    while (isAlpha(peek(scanner))) advanceScanner(scanner);
    TokenType type = specialType(scanner);
    if (type == TOKEN_ERROR) return errorToken(scanner, "Invalid special key");
    return makeToken(scanner, type);
}

static Token
getKey(Scanner* scanner, char c)
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
        Token result = getSpecialKey(&clone);
        if (result.type != TOKEN_ERROR)
        {
            cloneScanner(&clone, scanner);
            return result;
        }
    }

    return makeToken(scanner, TOKEN_KEY);
}

static Token
checkInterpolation(Scanner* scanner)
{
    assert(scanner);

    /* Seek to end of keyword. Only fails if given invalid CharType parameter. */
    if (seekToCharType(scanner, CHAR_TYPE_INTERP_END) == CHAR_TYPE_STATUS_ERROR)
    {
        return errorToken(scanner, "Internal error. Trying to seek to invalid char type.");
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
        if (isKeyword(scanner, 1, 3, "esc") || isKeyword(scanner, 1, 10, "escription"))
        {
            if (scanner->interpType == TOKEN_DESC_INTERP)
            {
                return errorToken(scanner, "Cannot interpolate description within a description.");
            }
            result = TOKEN_THIS_DESC;
        }
        break;
    }
    default: errorMsg("Unexpected inrepolation start: '%c'.", *scanner->start); break;
    }

    if (result == TOKEN_ERROR) return errorToken(scanner, "Got unexpected interpolation identifier.");
    return makeToken(scanner, result);
}

static Token
getInterpolation(Scanner* scanner)
{
    skipWhitespace(scanner);
    if (isAtEnd(scanner))
    {
        scanner->isInterpolation = false;
        return makeToken(scanner, TOKEN_EOF);
    }

    char c = advanceScanner(scanner);

    switch (c)
    {
    case ')':
        scanner->isInterpolation = false;
        makeScannerCurrent(scanner);
        if (scanner->interpType == TOKEN_DESC_INTERP)
        {
            return getDescription(scanner);
        }
        else if (scanner->interpType == TOKEN_COMM_INTERP)
        {
            return getCommand(scanner);
        }
        return errorToken(scanner, "Not sure how we got here...");
    default:
        return checkInterpolation(scanner);
    }
}

Token
scanToken(Scanner* scanner)
{
    assert(scanner);

    if (scanner->isInterpolation) return getInterpolation(scanner);

    skipWhitespace(scanner);
    if (isAtEnd(scanner)) return makeToken(scanner, TOKEN_EOF);

    char c = advanceScanner(scanner);

    switch (c)
    {
    /* single characters */
    case '[': return makeToken(scanner, TOKEN_LEFT_BRACKET);
    case ']': return makeToken(scanner, TOKEN_RIGHT_BRACKET);
    case '{': return makeToken(scanner, TOKEN_LEFT_BRACE);
    case '}': return makeToken(scanner, TOKEN_RIGHT_BRACE);
    case '(': return makeToken(scanner, TOKEN_LEFT_PAREN);
    case ')': return makeToken(scanner, TOKEN_RIGHT_PAREN);

    /* Hooks, flags, and preprocessor commands */
    case '^': makeScannerCurrent(scanner); return getHook(scanner);
    case '+': makeScannerCurrent(scanner); return getFlag(scanner);
    case ':': makeScannerCurrent(scanner); return getPreprocessorCommand(scanner);

    /* literals */
    case '\"':
        makeScannerCurrent(scanner);
        return getDescription(scanner);
    case '%':
        if (peek(scanner) != '{')
        {
            return getKey(scanner, c);
        }
        else if (matchScanner(scanner, '{') && !matchScanner(scanner, '{'))
        {
            return errorToken(
                scanner, "Expected '{' after '%{'. '{' must be escaped if it is meant to be a key."
            );
        }

        makeScannerCurrent(scanner);
        skipWhitespace(scanner);
        return getCommand(scanner);
    case '\\': makeScannerCurrent(scanner); return getKey(scanner, advanceScanner(scanner));

    /* keys */
    case 'C': /* FALLTHROUGH */
    case 'H':
    case 'M':
    case 'S':
        if (matchScanner(scanner, '-')) return makeToken(scanner, getMod(c));
        return getKey(scanner, c);
    default: return getKey(scanner, c);
    }

    return errorToken(scanner, "Unexpected character");
}
