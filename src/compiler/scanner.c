#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* common includes */
#include "common/common.h"
#include "common/key_chord.h"

/* local includes */
#include "scanner.h"
#include "token.h"

typedef uint8_t CharType;
enum
{
    CHAR_TYPE_WHITESPACE,
    CHAR_TYPE_INTERP_END,
};

void
vscannerErrorAt(Scanner* scanner, Token* token, const char* fmt, va_list ap)
{
    assert(scanner), assert(token), assert(fmt);

    tokenErrorAt(token, scanner->filepath);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
}

void
scannerErrorAt(Scanner* scanner, Token* token, const char* fmt, ...)
{
    assert(scanner), assert(token), assert(fmt);

    va_list ap;
    va_start(ap, fmt);
    vscannerErrorAt(scanner, token, fmt, ap);
    va_end(ap);
}

void
scannerInit(Scanner* scanner, const char* source, const char* filepath)
{
    assert(scanner), assert(source);

    scanner->head          = source;
    scanner->start         = source;
    scanner->current       = source;
    scanner->filepath      = filepath;
    scanner->line          = 1;
    scanner->column        = 0;
    scanner->state         = SCANNER_STATE_NORMAL;
    scanner->previousState = SCANNER_STATE_NORMAL;
    scanner->interpType    = TOKEN_EMPTY;
    scanner->hadError      = false;
}

void
scannerClone(const Scanner* scanner, Scanner* clone)
{
    assert(scanner), assert(clone);

    clone->head          = scanner->head;
    clone->start         = scanner->start;
    clone->current       = scanner->current;
    clone->filepath      = scanner->filepath;
    clone->line          = scanner->line;
    clone->column        = scanner->column;
    clone->hadError      = scanner->hadError;
    clone->state         = scanner->state;
    clone->previousState = scanner->previousState;
    clone->interpType    = scanner->interpType;
}

void
scannerMakeCurrent(Scanner* scanner)
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
scannerIsAtEnd(const Scanner* scanner)
{
    assert(scanner);

    return *scanner->current == '\0';
}

static void
updateLineAndColumn(Scanner* scanner, char c)
{
    assert(scanner);

    switch (c)
    {
    case '\n':
        scanner->line++;
        scanner->column = 0;
        break;
    default: scanner->column++; break;
    }
}

static char
advance(Scanner* scanner)
{
    assert(scanner);

    updateLineAndColumn(scanner, *scanner->current++);
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

    if (scannerIsAtEnd(scanner)) return '\0';
    return scanner->current[1];
}

static char
peekStart(const Scanner* scanner)
{
    assert(scanner);

    return *scanner->start;
}

static bool
match(Scanner* scanner, const char expected)
{
    assert(scanner);

    if (scannerIsAtEnd(scanner)) return false;
    if (peek(scanner) != expected) return false;
    advance(scanner);
    return true;
}

static bool
consume(Scanner* scanner, const char expected)
{
    assert(scanner);

    return expected == advance(scanner);
}

static void
tokenMake(const Scanner* scanner, Token* token, const TokenType type)
{
    assert(scanner), assert(token);

    token->type   = type;
    token->start  = scanner->start;
    token->length = (int)(scanner->current - scanner->start);
    token->line   = scanner->line;
    token->column = scanner->column;
    /* error */
    token->message       = token->start;
    token->messageLength = token->length;
}

static void
tokenMakeError(const Scanner* scanner, Token* token, const char* message)
{
    assert(scanner), assert(token);

    token->type   = TOKEN_ERROR;
    token->start  = scanner->start;
    token->length = (int)(scanner->current - scanner->start);
    token->line   = scanner->line;
    token->column = scanner->column;
    /* error */
    token->message       = message;
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
        case ' ':
        case '\r':
        case '\t': advance(scanner); break;
        case '#':
        {
            while (peek(scanner) != '\n' && !scannerIsAtEnd(scanner))
                advance(scanner);
            break;
        }
        default: scannerMakeCurrent(scanner); return;
        }
    }
}

static bool
isKeyword(Scanner* scanner, int start, int length, const char* rest)
{
    assert(scanner);

    return (
        scanner->current - scanner->start == start + length &&
        memcmp(scanner->start + start, rest, length) == 0);
}

static bool
seekToCharType(Scanner* scanner, CharType charType)
{
    assert(scanner);

    switch (charType)
    {
    case CHAR_TYPE_WHITESPACE:
    {
        while (!scannerIsAtEnd(scanner) && !isspace(peek(scanner)))
            advance(scanner);
        return !scannerIsAtEnd(scanner);
    }
    case CHAR_TYPE_INTERP_END:
    {
        while (!scannerIsAtEnd(scanner) && peek(scanner) != ')')
            advance(scanner);
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
        return tokenMakeError(scanner, token, "Got end of file while scanning hook keyword.");
    }

    TokenType result = TOKEN_ERROR;

    /* Switch on start of keyword */
    switch (peekStart(scanner))
    {
    case 'a':
        if (isKeyword(scanner, 1, 4, "fter")) result = TOKEN_AFTER;
        break;
    case 'b':
        if (isKeyword(scanner, 1, 5, "efore")) result = TOKEN_BEFORE;
        break;
    case 's':
    {
        if (isKeyword(scanner, 1, 10, "ync-before")) result = TOKEN_SYNC_BEFORE;
        if (isKeyword(scanner, 1, 9, "ync-after")) result = TOKEN_SYNC_AFTER;
        break;
    }
    default: break;
    }

    if (result == TOKEN_ERROR)
        return tokenMakeError(scanner, token, "Got unexpected hook keyword.");
    return tokenMake(scanner, token, result);
}

static void
scanFlag(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    /* Seek to end of keyword. Only fails if given invalid CharType parameter. */
    if (!seekToCharType(scanner, CHAR_TYPE_WHITESPACE))
    {
        return tokenMakeError(scanner, token, "Got end of file while scanning flag keyword.");
    }

    TokenType result = TOKEN_ERROR;

    /* Switch on start of keyword */
    switch (peekStart(scanner))
    {
    case 'k':
    {
        if (isKeyword(scanner, 1, 3, "eep")) result = TOKEN_KEEP;
        break;
    }
    case 'c':
    {
        if (isKeyword(scanner, 1, 4, "lose")) result = TOKEN_CLOSE;
        break;
    }
    case 'd':
    {
        if (isKeyword(scanner, 1, 5, "eflag")) result = TOKEN_DEFLAG;
        break;
    }
    case 'e':
    {
        if (isKeyword(scanner, 1, 6, "xecute")) result = TOKEN_EXECUTE;
        break;
    }
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
    case 'u':
    {
        if (isKeyword(scanner, 1, 5, "nhook")) result = TOKEN_UNHOOK;
        else if (isKeyword(scanner, 1, 5, "nwrap")) result = TOKEN_UNWRAP;
        break;
    }
    case 'w':
    {
        if (isKeyword(scanner, 1, 4, "rite")) result = TOKEN_WRITE;
        else if (isKeyword(scanner, 1, 3, "rap")) result = TOKEN_WRAP;
        break;
    }
    case 's':
    {
        if (isKeyword(scanner, 1, 11, "ync-command")) result = TOKEN_SYNC_CMD;
        break;
    }
    case 't':
    {
        if (isKeyword(scanner, 1, 4, "itle")) result = TOKEN_TITLE;
        break;
    }
    default: break;
    }

    if (result == TOKEN_ERROR)
    {
        return tokenMakeError(scanner, token, "Got unexpected flag keyword.");
    }
    return tokenMake(scanner, token, result);
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
    case 'b':
    {
        if (isKeyword(scanner, 1, 5, "ottom")) result = TOKEN_BOTTOM;
        else if (isKeyword(scanner, 1, 11, "order-width")) result = TOKEN_BORDER_WIDTH;
        else if (isKeyword(scanner, 1, 12, "order-radius")) result = TOKEN_BORDER_RADIUS;
        else if (isKeyword(scanner, 1, 1, "g")) result = TOKEN_BACKGROUND_COLOR;
        else if (isKeyword(scanner, 1, 1, "d")) result = TOKEN_BORDER_COLOR;
        break;
    }
    case 'd':
    {
        if (isKeyword(scanner, 1, 4, "ebug")) result = TOKEN_DEBUG;
        else if (isKeyword(scanner, 1, 4, "elay")) result = TOKEN_MENU_DELAY;
        else if (isKeyword(scanner, 1, 8, "elimiter")) result = TOKEN_DELIMITER;
        break;
    }
    case 'f':
    {
        if (isKeyword(scanner, 1, 1, "g")) result = TOKEN_FOREGROUND_COLOR;
        else if (isKeyword(scanner, 1, 5, "g-key")) result = TOKEN_FOREGROUND_KEY_COLOR;
        else if (isKeyword(scanner, 1, 11, "g-delimiter")) result = TOKEN_FOREGROUND_DELIMITER_COLOR;
        else if (isKeyword(scanner, 1, 8, "g-prefix")) result = TOKEN_FOREGROUND_PREFIX_COLOR;
        else if (isKeyword(scanner, 1, 7, "g-chord")) result = TOKEN_FOREGROUND_CHORD_COLOR;
        else if (isKeyword(scanner, 1, 7, "g-title")) result = TOKEN_FOREGROUND_TITLE_COLOR;
        else if (isKeyword(scanner, 1, 3, "ont")) result = TOKEN_FONT;
        break;
    }
    case 'h':
    {
        if (isKeyword(scanner, 1, 13, "eight-padding")) result = TOKEN_HEIGHT_PADDING;
        break;
    }
    case 'i':
    {
        if (isKeyword(scanner, 1, 6, "nclude")) result = TOKEN_INCLUDE;
        else if (isKeyword(scanner, 1, 18, "mplicit-array-keys")) result = TOKEN_IMPLICIT_ARRAY_KEYS;
        break;
    }
    case 'k':
    {
        if (isKeyword(scanner, 1, 9, "eep-delay")) result = TOKEN_KEEP_DELAY;
        break;
    }
    case 'm':
    {
        if (isKeyword(scanner, 1, 10, "ax-columns")) result = TOKEN_MAX_COLUMNS;
        else if (isKeyword(scanner, 1, 9, "enu-width")) result = TOKEN_MENU_WIDTH;
        else if (isKeyword(scanner, 1, 7, "enu-gap")) result = TOKEN_MENU_GAP;
        break;
    }
    case 's':
    {
        if (isKeyword(scanner, 1, 4, "hell")) result = TOKEN_SHELL;
        break;
    }
    case 't':
    {
        if (isKeyword(scanner, 1, 2, "op")) result = TOKEN_TOP;
        else if (isKeyword(scanner, 1, 4, "itle")) result = TOKEN_MENU_TITLE;
        else if (isKeyword(scanner, 1, 9, "itle-font")) result = TOKEN_MENU_TITLE_FONT;
        else if (isKeyword(scanner, 1, 12, "able-padding")) result = TOKEN_TABLE_PADDING;
        break;
    }
    case 'u':
    {
        if (isKeyword(scanner, 1, 7, "nsorted")) result = TOKEN_UNSORTED;
        break;
    }
    case 'v':
    {
        if (isKeyword(scanner, 1, 2, "ar")) result = TOKEN_VAR;
        break;
    }
    case 'w':
    {
        if (isKeyword(scanner, 1, 12, "idth-padding")) result = TOKEN_WIDTH_PADDING;
        else if (isKeyword(scanner, 1, 7, "rap-cmd")) result = TOKEN_WRAP_CMD;
        break;
    }
    default: break;
    }

    if (result == TOKEN_ERROR)
    {
        return tokenMakeError(scanner, token, "Got unexpected preprocessor command.");
    }
    return tokenMake(scanner, token, result);
}

static void
scanMetaCmd(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    seekToCharType(scanner, CHAR_TYPE_WHITESPACE);

    TokenType result = TOKEN_ERROR;

    /* Switch on start of keyword */
    switch (peekStart(scanner))
    {
    case 'g':
    {
        if (isKeyword(scanner, 1, 3, "oto")) result = TOKEN_GOTO;
        break;
    }
    default: break;
    }

    if (result == TOKEN_ERROR)
    {
        return tokenMakeError(scanner, token, "Got unexpected meta command.");
    }
    return tokenMake(scanner, token, result);
}

static TokenType
getInterpolationType(Scanner* scanner)
{
    assert(scanner);

    Scanner clone = { 0 };
    scannerClone(scanner, &clone);

    if (match(&clone, '%') && !match(&clone, '('))
    {
        errorMsg("Internal error. Invalid call to `isInterpolation`.");
        return TOKEN_ERROR;
    }

    scannerMakeCurrent(&clone);
    if (!seekToCharType(&clone, CHAR_TYPE_INTERP_END)) return TOKEN_ERROR;

    /* Switch on start of keyword */
    switch (peekStart(&clone))
    {
    case 'k':
    {
        if (isKeyword(&clone, 1, 2, "ey")) return TOKEN_THIS_KEY;
        break;
    }
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
    case 'w':
    {
        if (isKeyword(&clone, 1, 7, "rap_cmd")) return TOKEN_WRAP_CMD_INTERP;
        break;
    }
    default: break;
    }

    if (!scannerIsAtEnd(&clone) && peek(&clone) == ')')
    {
        return TOKEN_USER_VAR;
    }

    return TOKEN_ERROR;
}

static void
scanDescription(Scanner* scanner, Token* token, bool allowInterpolation)
{
    assert(scanner), assert(token);

    while (!scannerIsAtEnd(scanner))
    {
        switch (peek(scanner))
        {
        case '\"':
        {
            tokenMake(scanner, token, TOKEN_DESCRIPTION);
            consume(scanner, '\"');
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
                return tokenMakeError(
                    scanner,
                    token,
                    "Cannot interpolate the description within the description.");
            }
            /* Not an interpolation */
            case TOKEN_ERROR: break;
            /* Is a valid interpolation. */
            default:
            {
                scanner->previousState = SCANNER_STATE_DESCRIPTION;
                scanner->state         = SCANNER_STATE_INTERPOLATION;
                tokenMake(scanner, token, TOKEN_DESC_INTERP);
                consume(scanner, '%');
                consume(scanner, '(');
                scannerMakeCurrent(scanner);
                return;
            }
            }
            scanner->interpType = TOKEN_EMPTY;
            break;
        }
        case '\\':
            if (peekNext(scanner) == '\"') consume(scanner, '\\');
            break;
        }
        advance(scanner);
    }

    scanner->state = SCANNER_STATE_NORMAL;
    return tokenMakeError(scanner, token, "Unterminated string");
}

static void
scanCommand(Scanner* scanner, Token* token, char delim, bool allowInterpolation)
{
    assert(scanner), assert(token);

    static char delimiter = '\0';

    if (delim != '\0')
    {
        switch (delim)
        {
        case '{': delimiter = '}'; break;
        case '(': delimiter = ')'; break;
        case '[': delimiter = ']'; break;
        default: delimiter = delim; break;
        }
    }

    while (!scannerIsAtEnd(scanner))
    {
        char c = peek(scanner);

        if (c == delimiter && peekNext(scanner) == delimiter)
        {
            tokenMake(scanner, token, TOKEN_COMMAND);
            consume(scanner, delimiter);
            consume(scanner, delimiter);
            scanner->state = SCANNER_STATE_NORMAL;
            return;
        }

        if (allowInterpolation && c == '%' && peekNext(scanner) == '(')
        {
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
                scanner->state         = SCANNER_STATE_INTERPOLATION;
                tokenMake(scanner, token, TOKEN_COMM_INTERP);
                consume(scanner, '%');
                consume(scanner, '(');
                scannerMakeCurrent(scanner);
                return;
            }
            }
            scanner->interpType = TOKEN_EMPTY;
        }

        advance(scanner);
    }

    scanner->state = SCANNER_STATE_NORMAL;
    return tokenMakeError(scanner, token, "Expected delimiter after command but got end of file");
}

static TokenType
scanMod(const char c)
{
    switch (c)
    {
    case 'C': return TOKEN_MOD_CTRL;
    case 'H': return TOKEN_MOD_HYPER;
    case 'M': return TOKEN_MOD_META;
    case 'S': return TOKEN_MOD_SHIFT;
    default: return TOKEN_ERROR;
    }
}

static TokenType
checkSpecialType(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    for (size_t i = SPECIAL_KEY_NONE; i < SPECIAL_KEY_LAST; i++)
    {
        const char* repr = specialKeyRepr(i);
        if (isKeyword(
                scanner,
                0,
                strlen(repr + 0),
                repr + 0)) /* Check the whole token, not n - 1 */
        {
            token->special = i;
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

    TokenType type = checkSpecialType(scanner, token);
    if (type == TOKEN_ERROR) return tokenMakeError(scanner, token, "Invalid special key");
    return tokenMake(scanner, token, type);
}

static void
scanKey(Scanner* scanner, Token* token, char c)
{
    assert(scanner), assert(token);

    if (isUtf8MultiByteStartByte(c))
    {
        /* NOTE scanning multi byte character */
        while (isUtf8ContByte(peek(scanner)))
            c = advance(scanner);
    }
    else
    {
        /* NOTE possible special character */
        Scanner clone;
        scannerClone(scanner, &clone);
        scanSpecialKey(&clone, token);
        if (token->type == TOKEN_SPECIAL_KEY) return scannerClone(&clone, scanner);
    }

    return tokenMake(scanner, token, TOKEN_KEY);
}

static void
scanInterpolation(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    if (!seekToCharType(scanner, CHAR_TYPE_INTERP_END))
    {
        return tokenMakeError(
            scanner,
            token,
            "Internal error. Got invalid call to `scanInterpolation`.");
    }

    /* Restore previous state to keep scanning for description or command. */
    scanner->state = scanner->previousState;

    /* The interpType should already be set by the previous call to checkInterpolation. */
    if (scanner->interpType == TOKEN_ERROR || scanner->interpType == TOKEN_EMPTY)
    {
        tokenMakeError(scanner, token, "Internal error. Got invalid interp type.");
    }
    else
    {
        tokenMake(scanner, token, scanner->interpType);
    }
    scanner->interpType = TOKEN_EMPTY;

    /* Consume the interpolation */
    consume(scanner, ')');
    scannerMakeCurrent(scanner);
    return;
}

static void
scanDouble(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    while (isDigit(peek(scanner)))
        advance(scanner);
    if (peek(scanner) == '.')
    {
        advance(scanner);
        while (isDigit(peek(scanner)))
            advance(scanner);
    }

    return tokenMake(scanner, token, TOKEN_DOUBLE);
}

static void
scanInteger(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    while (isDigit(peek(scanner)))
        advance(scanner);

    return tokenMake(scanner, token, TOKEN_INTEGER);
}

static void
scanUnsignedInteger(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    while (isDigit(peek(scanner)))
        advance(scanner);

    return tokenMake(scanner, token, TOKEN_UNSIGNED_INTEGER);
}

void
scannerTokenForCompiler(Scanner* scanner, Token* token)
{
    assert(scanner), assert(token);

    tokenInit(token);

    switch (scanner->state)
    {
    case SCANNER_STATE_COMMAND: return scanCommand(scanner, token, '\0', true);
    case SCANNER_STATE_DESCRIPTION: return scanDescription(scanner, token, true);
    case SCANNER_STATE_INTERPOLATION: return scanInterpolation(scanner, token);
    default: break;
    }

    skipWhitespace(scanner);
    if (scannerIsAtEnd(scanner)) return tokenMake(scanner, token, TOKEN_EOF);

    char c = advance(scanner);

    switch (c)
    {
    /* single characters */
    case '[': return tokenMake(scanner, token, TOKEN_LEFT_BRACKET);
    case ']': return tokenMake(scanner, token, TOKEN_RIGHT_BRACKET);
    case '{': return tokenMake(scanner, token, TOKEN_LEFT_BRACE);
    case '}': return tokenMake(scanner, token, TOKEN_RIGHT_BRACE);
    case '(': return tokenMake(scanner, token, TOKEN_LEFT_PAREN);
    case ')': return tokenMake(scanner, token, TOKEN_RIGHT_PAREN);
    case '.':
    {
        if (peek(scanner) != '.' || peekNext(scanner) != '.') return scanKey(scanner, token, c);
        consume(scanner, '.');
        consume(scanner, '.');
        return tokenMake(scanner, token, TOKEN_ELLIPSIS);
    }

    /* Hooks, flags, meta commands, and preprocessor commands */
    case '^': scannerMakeCurrent(scanner); return scanHook(scanner, token);
    case '+': scannerMakeCurrent(scanner); return scanFlag(scanner, token);
    /* TODO: should we detect preprocessor commands when scanning for the compiler?? */
    case ':': scannerMakeCurrent(scanner); return scanPreprocessorMacro(scanner, token);
    case '@': scannerMakeCurrent(scanner); return scanMetaCmd(scanner, token);

    /* literals */
    case '\"':
    {
        scannerMakeCurrent(scanner);
        return scanDescription(scanner, token, true);
    }
    case '%':
    {
        char delim = peek(scanner);

        if (match(scanner, delim) && !match(scanner, delim))
        {
            return tokenMakeError(scanner, token, "Expected delimiter after '%'.");
        }

        scanner->state = SCANNER_STATE_COMMAND;
        scannerMakeCurrent(scanner);
        return scanCommand(scanner, token, delim, true);
    }
    case '\\': scannerMakeCurrent(scanner); return scanKey(scanner, token, advance(scanner));

    /* keys */
    case 'C': /* FALLTHROUGH */
    case 'H':
    case 'M':
    case 'S':
    {
        if (match(scanner, '-')) return tokenMake(scanner, token, scanMod(c));
        return scanKey(scanner, token, c);
    }
    default: return scanKey(scanner, token, c);
    }

    return tokenMakeError(scanner, token, "Unreachable character");
}

void
scannerTokenForPreprocessor(Scanner* scanner, Token* token, ScannerFlag flag)
{
    assert(scanner), assert(token);

    tokenInit(token);

    /* Handle scanner state for interpolations, descriptions, and commands */
    switch (scanner->state)
    {
    case SCANNER_STATE_DESCRIPTION: return scanDescription(scanner, token, true);
    case SCANNER_STATE_INTERPOLATION: return scanInterpolation(scanner, token);
    default: break;
    }

    while (!scannerIsAtEnd(scanner))
    {
        char c = advance(scanner);
        switch (c)
        {
        case '#':
        {
            while (!scannerIsAtEnd(scanner))
            {
                if (advance(scanner) == '\n') break;
            }
            break;
        }
        case '%':
        {
            /* Skip over any false positives preproccessor macros in commands */
            if (match(scanner, '{') && match(scanner, '{'))
            {
                while (!scannerIsAtEnd(scanner))
                {
                    char e = advance(scanner);
                    if (e == '}' && match(scanner, '}')) break;
                }
            }
            break;
        }
        case ':': scannerMakeCurrent(scanner); return scanPreprocessorMacro(scanner, token);
        case '\"':
        {
            /* Don't scan ad a description unless requsted */
            if (flag != SCANNER_WANTS_DESCRIPTION)
            {
                /* Skip over any false positives preproccessor macros in descriptions */
                while (!scannerIsAtEnd(scanner))
                {
                    char e = advance(scanner);
                    if (e == '\\')
                        advance(scanner);
                    else if (e == '\"')
                        break;
                }
                break;
            }
            scannerMakeCurrent(scanner);
            return scanDescription(scanner, token, true);
        }
        default:
        {
            /* Do nothing if scanner not searching for number. */
            if (flag != SCANNER_WANTS_DOUBLE && flag != SCANNER_WANTS_INTEGER &&
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

    return tokenMake(scanner, token, TOKEN_EOF);
}
