#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include "lib/util.h"

#include "scanner.h"

void
initScanner(Scanner* scanner, const char* source)
{
    assert(scanner && source);

    scanner->start = source;
    scanner->current = source;
    scanner->line = 1;
    scanner->interpType = TOKEN_NO_INTERP;
    scanner->isInterpolation = false;
}

static void
cloneScanner(Scanner* scanner, Scanner* clone)
{
    clone->start = scanner->start;
    clone->current = scanner->current;
    clone->line = scanner->line;
    clone->interpType = scanner->interpType;
    clone->isInterpolation = scanner->isInterpolation;
}

static void
makeCurrent(Scanner* scanner)
{
    scanner->start = scanner->current;
}

static bool
isDigit(const char c)
{
    return '0' <= c && c <= '9';
}

static bool
isAlpha(const char c)
{
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
            c == '+' || c == '-' || c == '_';
}

static bool
isAtEnd(const Scanner* scanner)
{
    return *scanner->current == '\0';
}

static char
advance(Scanner* scanner)
{
    scanner->current++;
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
match(Scanner* scanner, const char expected)
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

static void
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
            advance(scanner);
            break;
        case '#':
            while (peek(scanner) != '\n' && !isAtEnd(scanner)) advance(scanner);
            break;
        default: makeCurrent(scanner); return;
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

static TokenType
checkKeyword(Scanner* scanner, size_t start, size_t length, const char* rest, TokenType type)
{
    if (isKeyword(scanner, start, length, rest)) return type;
    return TOKEN_ERROR;
}

static TokenType
identifierType(Scanner* scanner)
{
    switch (*scanner->start)
    {
    case 'a':
        if (isKeyword(scanner, 1, 4, "fter"))       return TOKEN_AFTER;
        if (isKeyword(scanner, 1, 11, "sync-before")) return TOKEN_ASYNC_BEFORE;
        break;
    case 'b': return checkKeyword(scanner, 1, 5, "efore", TOKEN_BEFORE);
    case 'c': return checkKeyword(scanner, 1, 4, "lose", TOKEN_CLOSE);
    case 'd':
        if (isKeyword(scanner, 1, 3, "esc") || isKeyword(scanner, 1, 10, "escription"))
        {
            if (scanner->interpType == TOKEN_DESC_INTERP) return TOKEN_NO_INTERP;
            return TOKEN_THIS_DESC;
        }
        break;
    case 'i':
        if (isKeyword(scanner, 1, 6, "ndex+1"))     return TOKEN_INDEX_ONE;
        if (isKeyword(scanner, 1, 4, "ndex"))       return TOKEN_INDEX;
        if (isKeyword(scanner, 1, 6, "nherit"))     return TOKEN_INHERIT;
        break;
    case 'k':
        if (isKeyword(scanner, 1, 3, "eep"))        return TOKEN_KEEP;
        if (isKeyword(scanner, 1, 2, "ey"))         return TOKEN_THIS_KEY;
        break;
    case 'n':
        if (isKeyword(scanner, 1, 7, "o-after"))    return TOKEN_NO_AFTER;
        if (isKeyword(scanner, 1, 8, "o-before"))   return TOKEN_NO_BEFORE;
        break;
    case 's': return checkKeyword(scanner, 1, 11, "ync-command", TOKEN_SYNC_CMD);
    case 'u': return checkKeyword(scanner, 1, 5, "nhook", TOKEN_UNHOOK);
    case 'w': return checkKeyword(scanner, 1, 4, "rite", TOKEN_WRITE);
    }
    return TOKEN_ERROR;
}

static Token
identifier(Scanner* scanner)
{
    while (isAlpha(peek(scanner)) || isDigit(peek(scanner))) advance(scanner);
    TokenType type = identifierType(scanner);
    if (type == TOKEN_ERROR) return errorToken(scanner, "Invalid keyword");
    if (type == TOKEN_NO_INTERP) return errorToken(scanner, "Cannot interpolate description within a description.");
    return makeToken(scanner, type);
}

static Token
description(Scanner* scanner)
{
    while (!isAtEnd(scanner))
    {
        switch (peek(scanner))
        {
        /* case '\'': /\* NOTE possible return *\/ */
        case '\"': /* NOTE possible return */
        {
            Token result = makeToken(scanner, TOKEN_DESCRIPTION);
            advance(scanner);
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
                advance(scanner); /* % */
                advance(scanner); /* ( */
                return result;
            }
            break;
        }
        case '\\':
            /* if (peekNext(scanner) == '\'') */
            if (peekNext(scanner) == '\"')
            {
                advance(scanner);
            }
            break;
        }
        advance(scanner);
    }

    return errorToken(scanner, "Unterminated string");
}

static Token
command(Scanner* scanner)
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
                advance(scanner); /* '}' */
                advance(scanner); /* '}' */
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
                advance(scanner); /* % */
                advance(scanner); /* ( */
                return result;
            }
            break;
        }
        advance(scanner);
    }

    return errorToken(scanner, "Expected '}}' but got end of file");
}

static TokenType
checkMod(const char c)
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
specialKey(Scanner* scanner)
{
    while (isAlpha(peek(scanner))) advance(scanner);
    TokenType type = specialType(scanner);
    if (type == TOKEN_ERROR) return errorToken(scanner, "Invalid special key");
    return makeToken(scanner, type);
}

static Token
key(Scanner* scanner, char c)
{
    if (isUtf8MultiByteStartByte(c))
    {
        /* NOTE scanning multi byte character */
        while (isUtf8ContByte(peek(scanner)))
        {
            c = advance(scanner);
        }
    }
    else
    {
        /* NOTE possible special character */
        Scanner clone;
        cloneScanner(scanner, &clone);
        Token result = specialKey(&clone);
        if (result.type != TOKEN_ERROR)
        {
            cloneScanner(&clone, scanner);
            return result;
        }
    }

    return makeToken(scanner, TOKEN_KEY);
}

static Token
scanInterpolation(Scanner* scanner)
{
    skipWhitespace(scanner);
    if (isAtEnd(scanner))
    {
        scanner->isInterpolation = false;
        return makeToken(scanner, TOKEN_EOF);
    }

    char c = advance(scanner);

    switch (c)
    {
    case ')':
        scanner->isInterpolation = false;
        makeCurrent(scanner);
        if (scanner->interpType == TOKEN_DESC_INTERP)
        {
            return description(scanner);
        }
        else if (scanner->interpType == TOKEN_COMM_INTERP)
        {
            return command(scanner);
        }
        return errorToken(scanner, "Not sure how we got here...");
    default:
        return identifier(scanner);
    }
}

Token
scanToken(Scanner* scanner)
{
    assert(scanner);

    if (scanner->isInterpolation) return scanInterpolation(scanner);

    skipWhitespace(scanner);
    if (isAtEnd(scanner)) return makeToken(scanner, TOKEN_EOF);

    char c = advance(scanner);

    switch (c)
    {
    /* single characters */
    case '[': return makeToken(scanner, TOKEN_LEFT_BRACKET);
    case ']': return makeToken(scanner, TOKEN_RIGHT_BRACKET);
    case '{': return makeToken(scanner, TOKEN_LEFT_BRACE);
    case '}': return makeToken(scanner, TOKEN_RIGHT_BRACE);

    /* keywords */
    case ':': makeCurrent(scanner); return identifier(scanner);

    /* literals */
    /* case '\'': */
    case '\"':
        makeCurrent(scanner);
        return description(scanner);
    case '%':
        if (!match(scanner, '{') || !match(scanner, '{'))
        {
            return errorToken(scanner, "Expect '{{' after '%'");
        }
        makeCurrent(scanner);
        skipWhitespace(scanner);
        return command(scanner);
    case '\\': makeCurrent(scanner); return key(scanner, advance(scanner));
    /* keys */
    case 'C': /* FALLTHROUGH */
    case 'H':
    case 'M':
    case 'S':
        if (match(scanner, '-'))
        {
            return makeToken(scanner, checkMod(c));
        }
        return key(scanner, c);
    default: return key(scanner, c);
    }

    return errorToken(scanner, "Unexpected character");
}
