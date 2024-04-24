#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

/* common includes */
#include "common/common.h"
#include "common/key_chord.h"

/* local includes */
#include "scanner.h"
#include "token.h"

typedef enum
{
    CHAR_TYPE_WHITESPACE,
    CHAR_TYPE_INTERP_END,
} CharType;

void
initScanner(Scanner* scanner, const char* source, const char* filepath)
{
    assert(scanner), assert(source);

    scanner->head = source;
    scanner->start = source;
    scanner->current = source;
    scanner->filepath = filepath;
    scanner->line = 1;
    scanner->column = 0;
    scanner->hadError = false;
    scanner->special = SPECIAL_KEY_NONE;
    scanner->state = SCANNER_STATE_NORMAL;
    scanner->previousState = SCANNER_STATE_NORMAL;
    scanner->interpType = TOKEN_EMPTY;
}

static void
cloneScanner(Scanner* scanner, Scanner* clone)
{
    assert(scanner), assert(clone);

    clone->head = scanner->head;
    clone->start = scanner->start;
    clone->current = scanner->current;
    clone->filepath = scanner->filepath;
    clone->line = scanner->line;
    clone->column = scanner->column;
    clone->hadError = scanner->hadError;
    clone->special = scanner->special;
    clone->state = scanner->state;
    clone->previousState = scanner->previousState;
    clone->interpType = scanner->interpType;
}

void
makeScannerCurrent(Scanner* scanner)
{
    assert(scanner);

    scanner->start = scanner->current;
}

static bool
isDigit(char c)
{
    return c >= '0' && c <= '9';
}

bool
isAtEnd(const Scanner* scanner)
{
    assert(scanner);

    return *scanner->current == '\0';
}

static void
scannerUpdateLineAndColumn(Scanner* scanner, char c)
{
    assert(scanner);

    switch (c)
    {
    case '\n': scanner->line++; scanner->column = 0; break;
    default: scanner->column++; break;
    }
}

static char
advanceScanner(Scanner* scanner)
{
    assert(scanner);

    scannerUpdateLineAndColumn(scanner, *scanner->current++);
    return scanner->current[-1];
}

static char
peek(const Scanner* scanner)
{
    assert(scanner);

    return *scanner->current;
}

static char
peekNext(const Scanner* scanner)
{
    assert(scanner);

    if (isAtEnd(scanner)) return '\0';
    return scanner->current[1];
}

static char
peekStart(const Scanner* scanner)
{
    assert(scanner);

    return *scanner->start;
}

static bool
matchScanner(Scanner* scanner, const char expected)
{
    assert(scanner);

    if (isAtEnd(scanner)) return false;
    if (peek(scanner) != expected) return false;
    advanceScanner(scanner);
    return true;
}

static bool
consumeScanner(Scanner* scanner, const char expected)
{
    assert(scanner);

    return expected == advanceScanner(scanner);
}

static void
makeToken(const Scanner* scanner, Token* token, const TokenType type)
{
    assert(scanner), assert(token);

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
    assert(scanner), assert(token);

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
    assert(scanner);

    while (true)
    {
        char c = peek(scanner);
        switch (c)
        {
        case '\n': /* FALLTHROUGH */
        case ' ' :
        case '\r':
        case '\t': advanceScanner(scanner); break;
        case '#':
        {
            while (peek(scanner) != '\n' && !isAtEnd(scanner)) advanceScanner(scanner);
            break;
        }
        default: makeScannerCurrent(scanner); return;
        }
    }
}

static bool
isKeyword(Scanner* scanner, int start, int length, const char* rest)
{
    assert(scanner);

    return (
        scanner->current - scanner->start == start + length &&
        memcmp(scanner->start + start, rest, length) == 0
    );
}

static bool
seekToCharType(Scanner* scanner, CharType charType)
{
    assert(scanner);

    switch (charType)
    {
    case CHAR_TYPE_WHITESPACE:
    {
        while (!isAtEnd(scanner) && !isspace(peek(scanner))) advanceScanner(scanner);
        return !isAtEnd(scanner);
    }
    case CHAR_TYPE_INTERP_END:
    {
        while (!isAtEnd(scanner) && peek(scanner) != ')') advanceScanner(scanner);
        return peek(scanner) == ')';
    }
    default: return false;
    }
}

static void
scanHook(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    /* Seek to end of keyword. Only fails if given invalid CharType parameter. */
    if (!seekToCharType(scanner, CHAR_TYPE_WHITESPACE))
    {
        return errorToken(scanner, token, "Got end of file while scanning hook keyword.");
    }

    TokenType result = TOKEN_ERROR;

    /* Switch on start of keyword */
    switch (peekStart(scanner))
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
scanFlag(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    /* Seek to end of keyword. Only fails if given invalid CharType parameter. */
    if (!seekToCharType(scanner, CHAR_TYPE_WHITESPACE))
    {
        return errorToken(scanner, token, "Got end of file while scanning flag keyword.");
    }

    TokenType result = TOKEN_ERROR;

    /* Switch on start of keyword */
    switch (peekStart(scanner))
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
scanPreprocessorMacro(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    /* Seek to end of keyword. Not an error if this is the last thing in the file. */
    seekToCharType(scanner, CHAR_TYPE_WHITESPACE);

    TokenType result = TOKEN_ERROR;

    /* Switch on start of keyword */
    switch (peekStart(scanner))
    {
    case 'd': if (isKeyword(scanner, 1, 4, "ebug")) result = TOKEN_DEBUG; break;
    case 'i': if (isKeyword(scanner, 1, 6, "nclude")) result = TOKEN_INCLUDE; break;
    case 't': if (isKeyword(scanner, 1, 2, "op")) result = TOKEN_TOP; break;
    case 'b':
    {
        if (isKeyword(scanner, 1, 5, "ottom")) result = TOKEN_BOTTOM;
        else if (isKeyword(scanner, 1, 11, "order-width")) result = TOKEN_BORDER_WIDTH;
        else if (isKeyword(scanner, 1, 12, "order-radius")) result = TOKEN_BORDER_RADIUS;
        else if (isKeyword(scanner, 1, 7, "g-color")) result = TOKEN_BACKGROUND_COLOR;
        else if (isKeyword(scanner, 1, 7, "d-color")) result = TOKEN_BORDER_COLOR;
        break;
    }
    case 'm':
    {
        if (isKeyword(scanner, 1, 10, "ax-columns")) result = TOKEN_MAX_COLUMNS;
        else if (isKeyword(scanner, 1, 9, "enu-width")) result = TOKEN_MENU_WIDTH;
        else if (isKeyword(scanner, 1, 7, "enu-gap")) result = TOKEN_MENU_GAP;
        break;
    }
    case 'w': if (isKeyword(scanner, 1, 12, "idth-padding")) result = TOKEN_WIDTH_PADDING; break;
    case 'h': if (isKeyword(scanner, 1, 13, "eight-padding")) result = TOKEN_HEIGHT_PADDING; break;
    case 'f':
    {
        if (isKeyword(scanner, 1, 1, "g")) result = TOKEN_FOREGROUND_COLOR;
        else if (isKeyword(scanner, 1, 5, "g-key")) result = TOKEN_FOREGROUND_KEY_COLOR;
        else if (isKeyword(scanner, 1, 7, "g-delimiter")) result = TOKEN_FOREGROUND_DELIMITER_COLOR;
        else if (isKeyword(scanner, 1, 8, "g-prefix")) result = TOKEN_FOREGROUND_PREFIX_COLOR;
        else if (isKeyword(scanner, 1, 7, "g-chord")) result = TOKEN_FOREGROUND_CHORD_COLOR;
        else if (isKeyword(scanner, 1, 3, "ont")) result = TOKEN_FONT;
        break;
    }
    case 's':
    {
        if (isKeyword(scanner, 1, 4, "hell")) result = TOKEN_SHELL;
        else if (isKeyword(scanner, 1, 3, "ort")) result = TOKEN_SORT;
        break;
    }
    default: break;
    }

    if (result == TOKEN_ERROR) return errorToken(scanner, token, "Got unexpected preprocessor command.");
    return makeToken(scanner, token, result);
}

static TokenType
getInterpolationType(Scanner* scanner)
{
    assert(scanner);

    Scanner clone = {0};
    cloneScanner(scanner, &clone);

    if (matchScanner(&clone, '%') && !matchScanner(&clone, '('))
    {
        errorMsg("Internal error. Invalid call to `isInterpolation`.");
        return TOKEN_ERROR;
    }

    makeScannerCurrent(&clone);
    if (!seekToCharType(&clone, CHAR_TYPE_INTERP_END)) return TOKEN_ERROR;

    /* Switch on start of keyword */
    switch (peekStart(&clone))
    {
    case 'k': if (isKeyword(&clone, 1, 2, "ey")) return TOKEN_THIS_KEY; break;
    case 'i':
    {
        if (isKeyword(&clone, 1, 4, "ndex")) return TOKEN_INDEX;
        else if (isKeyword(&clone, 1, 6, "ndex+1")) return TOKEN_INDEX_ONE;
        break;
    }
    case 'd':
    {
        if (isKeyword(&clone, 1, 3, "esc")) return TOKEN_THIS_DESC;
        else if (isKeyword(&clone, 1, 4, "esc^")) return TOKEN_THIS_DESC_UPPER_FIRST;
        else if (isKeyword(&clone, 1, 4, "esc,")) return TOKEN_THIS_DESC_LOWER_FIRST;
        else if (isKeyword(&clone, 1, 5, "esc^^")) return TOKEN_THIS_DESC_UPPER_ALL;
        else if (isKeyword(&clone, 1, 5, "esc,,")) return TOKEN_THIS_DESC_LOWER_ALL;
        break;
    }
    default: break;
    }

    return TOKEN_ERROR;
}

static void
scanDescription(Scanner* scanner, Token* token, bool allowInterpolation)
{
    assert(scanner), assert(token);

    while (!isAtEnd(scanner))
    {
        switch (peek(scanner))
        {
        case '\"':
        {
            makeToken(scanner, token, TOKEN_DESCRIPTION);
            consumeScanner(scanner, '\"');
            scanner->state = SCANNER_STATE_NORMAL;
            return;
        }
        case '%':
        {
            /* Don't check interpolation if not desired. */
            if (!allowInterpolation) break;

            /* Not an interpolation, keep going. */
            if (peekNext(scanner) != '(') break;

            /* Get the interpolation type, if any. */
            scanner->interpType = getInterpolationType(scanner);

            /* Switch on interpolation type. */
            switch (scanner->interpType)
            {
            /* Error */
            case TOKEN_THIS_DESC: /* FALLTHROUGH */
            case TOKEN_THIS_DESC_UPPER_FIRST:
            case TOKEN_THIS_DESC_LOWER_FIRST:
            case TOKEN_THIS_DESC_UPPER_ALL:
            case TOKEN_THIS_DESC_LOWER_ALL:
            {
                return errorToken(
                    scanner, token,
                    "Cannot interpolate the description within the description."
                );
            }
            /* Not an interpolation */
            case TOKEN_ERROR: break;
            /* Is a valid interpolation. */
            default:
            {
                scanner->previousState = SCANNER_STATE_DESCRIPTION;
                scanner->state = SCANNER_STATE_INTERPOLATION;
                makeToken(scanner, token, TOKEN_DESC_INTERP);
                consumeScanner(scanner, '%');
                consumeScanner(scanner, '(');
                makeScannerCurrent(scanner);
                return;
            }
            }
            scanner->interpType = TOKEN_EMPTY;
            break;
        }
        case '\\':
            if (peekNext(scanner) == '\"') consumeScanner(scanner, '\\');
            break;
        }
        advanceScanner(scanner);
    }

    scanner->state = SCANNER_STATE_NORMAL;
    return errorToken(scanner, token, "Unterminated string");
}

static void
scanCommand(Scanner* scanner, Token* token, bool allowInterpolation)
{
    assert(scanner), assert(token);

    while (!isAtEnd(scanner))
    {
        switch (peek(scanner))
        {
        case '}':
        {
            if (peekNext(scanner) == '}')
            {
                makeToken(scanner, token, TOKEN_COMMAND);
                consumeScanner(scanner, '}');
                consumeScanner(scanner, '}');
                scanner->state = SCANNER_STATE_NORMAL;
                return;
            }
            break;
        }
        case '%':
        {
            /* Don't check interpolation if not desired. */
            if (!allowInterpolation) break;

            /* Not an interpolation, keep going. */
            if (peekNext(scanner) != '(') break;

            /* Get the interpolation type, if any. */
            scanner->interpType = getInterpolationType(scanner);

            /* Switch on interpolation type. */
            switch (scanner->interpType)
            {
            /* Not an interpolation */
            case TOKEN_ERROR: break;
            /* Is a valid interpolation. */
            default:
            {
                scanner->previousState = SCANNER_STATE_COMMAND;
                scanner->state = SCANNER_STATE_INTERPOLATION;
                makeToken(scanner, token, TOKEN_COMM_INTERP);
                consumeScanner(scanner, '%');
                consumeScanner(scanner, '(');
                makeScannerCurrent(scanner);
                return;
            }
            }
            scanner->interpType = TOKEN_EMPTY;
            break;
        }
        }
        advanceScanner(scanner);
    }

    scanner->state = SCANNER_STATE_NORMAL;
    return errorToken(scanner, token, "Expected '}}' but got end of file");
}

static TokenType
scanMod(const char c)
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
checkSpecialType(Scanner* scanner)
{
    assert(scanner);

    for (size_t i = SPECIAL_KEY_NONE; i < SPECIAL_KEY_LAST; i++)
    {
        const char* repr = getSpecialKeyRepr(i);
        if (isKeyword(scanner, 1, strlen(repr + 1), repr + 1))
        {
            scanner->special = i;
            return TOKEN_SPECIAL_KEY;
        }
    }

    return TOKEN_ERROR;
}

static void
scanSpecialKey(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    /* Not an error if we get eof because this could be from `--press` which only has keys. */
    seekToCharType(scanner, CHAR_TYPE_WHITESPACE);

    TokenType type = checkSpecialType(scanner);
    if (type == TOKEN_ERROR) return errorToken(scanner, token, "Invalid special key");
    return makeToken(scanner, token, type);
}

static void
scanKey(Scanner* scanner, Token* token, char c)
{
    assert(scanner), assert(token);

    if (isUtf8MultiByteStartByte(c))
    {
        /* NOTE scanning multi byte character */
        while (isUtf8ContByte(peek(scanner))) c = advanceScanner(scanner);
    }
    else
    {
        /* NOTE possible special character */
        Scanner clone;
        cloneScanner(scanner, &clone);
        scanSpecialKey(&clone, token);
        if (token->type == TOKEN_SPECIAL_KEY) return cloneScanner(&clone, scanner);
    }

    return makeToken(scanner, token, TOKEN_KEY);
}

static void
scanInterpolation(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    if (!seekToCharType(scanner, CHAR_TYPE_INTERP_END))
    {
        return errorToken(
            scanner, token,
            "Internal error. Got invalid call to `scanInterpolation`."
        );
    }

    /* Restore previous state to keep scanning for description or command. */
    scanner->state = scanner->previousState;

    /* The interpType should already be set by the previous call to checkInterpolation. */
    if (scanner->interpType == TOKEN_ERROR || scanner->interpType == TOKEN_EMPTY)
    {
        errorToken(scanner, token, "Internal error. Got invalid interp type.");
    }
    else
    {
        makeToken(scanner, token, scanner->interpType);
    }
    scanner->interpType = TOKEN_EMPTY;

    /* Consume the interpolation */
    consumeScanner(scanner, ')');
    makeScannerCurrent(scanner);
    return;
}

static void
scanDouble(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    while (isDigit(peek(scanner))) advanceScanner(scanner);
    if (peek(scanner) == '.')
    {
        advanceScanner(scanner);
        while (isDigit(peek(scanner))) advanceScanner(scanner);
    }

    return makeToken(scanner, token, TOKEN_DOUBLE);
}

static void
scanInteger(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    while (isDigit(peek(scanner))) advanceScanner(scanner);

    return makeToken(scanner, token, TOKEN_INTEGER);
}

static void
scanUnsignedInteger(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    while (isDigit(peek(scanner))) advanceScanner(scanner);

    return makeToken(scanner, token, TOKEN_UNSIGNED_INTEGER);
}

void
scanTokenForCompiler(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    initToken(token);
    scanner->special = SPECIAL_KEY_NONE;

    switch (scanner->state)
    {
    case SCANNER_STATE_COMMAND: return scanCommand(scanner, token, true);
    case SCANNER_STATE_DESCRIPTION: return scanDescription(scanner, token, true);
    case SCANNER_STATE_INTERPOLATION: return scanInterpolation(scanner, token);
    default: break;
    }

    skipWhitespace(scanner);
    if (isAtEnd(scanner)) return makeToken(scanner, token, TOKEN_EOF);

    char c = advanceScanner(scanner);

    switch (c)
    {
    /* single characters */
    case '[': return makeToken(scanner, token, TOKEN_LEFT_BRACKET);
    case ']': return makeToken(scanner, token, TOKEN_RIGHT_BRACKET);
    case '{': return makeToken(scanner, token, TOKEN_LEFT_BRACE);
    case '}': return makeToken(scanner, token, TOKEN_RIGHT_BRACE);
    case '(': return makeToken(scanner, token, TOKEN_LEFT_PAREN);
    case ')': return makeToken(scanner, token, TOKEN_RIGHT_PAREN);

    /* Hooks, flags, and preprocessor commands */
    case '^': makeScannerCurrent(scanner); return scanHook(scanner, token);
    case '+': makeScannerCurrent(scanner); return scanFlag(scanner, token);
    case ':': makeScannerCurrent(scanner); return scanPreprocessorMacro(scanner, token);

    /* literals */
    case '\"':
    {
        makeScannerCurrent(scanner);
        return scanDescription(scanner, token, true);
    }
    case '%':
    {
        if (peek(scanner) != '{')
        {
            return scanKey(scanner, token, c);
        }
        else if (matchScanner(scanner, '{') && !matchScanner(scanner, '{'))
        {
            return errorToken(
                scanner, token, "Expected '{' after '%{'. '{' must be escaped if it is meant to be a key."
            );
        }

        scanner->state = SCANNER_STATE_COMMAND;
        makeScannerCurrent(scanner);
        return scanCommand(scanner, token, true);
    }
    case '\\': makeScannerCurrent(scanner); return scanKey(scanner, token, advanceScanner(scanner));

    /* keys */
    case 'C': /* FALLTHROUGH */
    case 'H':
    case 'M':
    case 'S':
    {
        if (matchScanner(scanner, '-')) return makeToken(scanner, token, scanMod(c));
        return scanKey(scanner, token, c);
    }
    default: return scanKey(scanner, token, c);
    }

    return errorToken(scanner, token, "Unreachable character");
}

void
scanTokenForPreprocessor(Scanner* scanner, Token* token, ScannerFlag flag)
{
    assert(scanner), assert(token);

    initToken(token);

    while (!isAtEnd(scanner))
    {
        char c = advanceScanner(scanner);
        switch (c)
        {
        case '#':
        {
            while (!isAtEnd(scanner))
            {
                if (advanceScanner(scanner) == '\n') break;
            }
            break;
        }
        case '%':
        {
            /* Skip over any false positives preproccessor macros in commands */
            if (matchScanner(scanner, '{') && matchScanner(scanner, '{'))
            {
                while (!isAtEnd(scanner))
                {
                    char e = advanceScanner(scanner);
                    if (e == '}' && matchScanner(scanner, '}')) break;
                }
            }
            break;
        }
        case ':': makeScannerCurrent(scanner); return scanPreprocessorMacro(scanner, token);
        case '\"':
        {
            /* Don't scan ad a description unless requsted */
            if (flag != SCANNER_WANTS_DESCRIPTION)
            {
                /* Skip over any false positives preproccessor macros in descriptions */
                while (!isAtEnd(scanner))
                {
                    char e = advanceScanner(scanner);
                    if (e == '\\') advanceScanner(scanner);
                    else if (e == '\"') break;
                }
                break;
            }
            makeScannerCurrent(scanner);
            return scanDescription(scanner, token, false);
        }
        default:
        {
            /* Do nothing if scanner not searching for number. */
            if (flag != SCANNER_WANTS_DOUBLE &&
                flag != SCANNER_WANTS_INTEGER &&
                flag != SCANNER_WANTS_UNSIGNED_INTEGER)
            {
                break;
            }

            if (flag == SCANNER_WANTS_INTEGER && (isDigit(c) || c == '-'))
            {
                return scanInteger(scanner, token);
            }
            else if (flag == SCANNER_WANTS_UNSIGNED_INTEGER && isDigit(c))
            {
                return scanUnsignedInteger(scanner, token);
            }
            else if (flag == SCANNER_WANTS_DOUBLE && isDigit(c))
            {
                return scanDouble(scanner, token);
            }
            break;
        }
        }
    }

    return makeToken(scanner, token, TOKEN_EOF);
}
