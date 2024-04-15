#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* common includes */
#include "common/common.h"
#include "common/debug.h"
#include "common/menu.h"
#include "common/string.h"

/* local includes */
#include "preprocessor.h"
#include "debug.h"
#include "piece_table.h"
#include "scanner.h"
#include "token.h"

static char*
getLitteralIncludePath(const char* start, size_t len)
{
    String result = {0};
    initString(&result);
    appendToString(&result, start, len);
    return result.string;
}

static size_t
getBaseDirLength(const char* sourcePath)
{
    assert(sourcePath);

    /* current moves through the 'sourcePath'
     * lastDir tracks the character past the last '/'
     * for 'sourcePath' '/home/john/wks/main.wks'
     * lastDir will point at the 'm' in main.
     * for 'sourcePath' 'wks/main.wks' the same is true.
     * for 'sourcePath' 'main.wks' it will still point at 'm'.
     * The function returns the length of the baseDir,
     * relative or absolute makes no difference here. */
    const char* current = sourcePath;
    const char* lastDir = sourcePath;

    while (*current != '\0')
    {
        char c = *current++;
        if (c == '/') lastDir = current;
    }

    return lastDir - sourcePath;
}

static char*
getIncludeFilePath(const char* start, size_t len, const char* sourcePath)
{
    /* If the include path is an absolute file, just copy the path directly. */
    if (*start == '/') return getLitteralIncludePath(start, len);

    /* NOTE could implement a different function to get the sourcePath
     * length i.e. '/home/john/wks/main.wks' would return the length
     * until the last '/' in the path. In this case it would be 15.
     * Because we have to check this in either case, there may be no
     * to distinguish an absolute vs relative sourcePath. */
    size_t baseLen = getBaseDirLength(sourcePath);

    /* sourcePath is a relative file from the PWD. Just use the path
     * from the include directive. */
    if (baseLen == 0) return getLitteralIncludePath(start, len);

    /* sourcePath is not in the PWD, return the sourcePath where the include
     * should be appended to. */
    String result = {0};
    initString(&result);
    appendToString(&result, sourcePath, baseLen);
    appendToString(&result, start, len);
    return result.string;
}

static void
handleIncludeMacro(WkMenu* menu, Scanner* scanner, PieceTable* result)
{
    assert(menu && scanner && result);

    /* currently pointing at the 'i' in ':include', so take off one. */
    /* const char* includeStart = scanner->start - 1; */
    const char* sourcePath = scanner->filepath;

    /* Ensure filename is given. */
    Token includeFile = {0};
    scanTokenForPreprocessor(scanner, &includeFile, SCANNER_WANTS_DESCRIPTION);
    if (includeFile.type != TOKEN_DESCRIPTION)
    {
        errorMsg("Expect \"FILEPATH\" after ':include'.");
        scanner->hadError = true;
        return;
    }

    if (!sourcePath) sourcePath = getenv("PWD");
    if (!sourcePath)
    {
        warnMsg("Could not get environment variable '$PWD' for script.");
    }

    /* /\* Append previous contents to result. *\/ */
    /* appendToPieceTable(result, PIECE_SOURCE_ORIGINAL, scannerStart, includeStart - scannerStart); */

    char* includeFilePath = NULL;
    char* includeSource = NULL; // readFile(includeFilePath);
    char* includeResult = NULL;

    /* Get the path to the included file */
    includeFilePath = getIncludeFilePath(includeFile.start, includeFile.length, sourcePath);
    if (!includeFilePath)
    {
        errorMsg("Failed to get the included file path.");
        scanner->hadError = true;
        goto fail;
    }

    /* Try to read the included file */
    includeSource = readFile(includeFilePath);
    if (!includeSource)
    {
        /* readFile prints an error for us. */
        scanner->hadError = true;
        goto fail;
    }

    /* check that the file and the source are not one and the same */
    if (strcmp(includeSource, scanner->head) == 0)
    {
        errorMsg(
            "Included file appears to be the same as the source file. Cannot `:include` self."
        );
        scanner->hadError = true;
        goto fail;
    }

    /* Run preprocessor on the included file */
    includeResult = runPreprocessor(menu, includeSource, includeFilePath);
    if (!includeResult)
    {
        errorMsg("Failed to get preprocessor result.");
        scanner->hadError = true;
        goto fail;
    }

    /* Append the result. */
    appendToPieceTable(result, PIECE_SOURCE_ADD, includeResult, strlen(includeResult));

fail:
    free(includeFilePath);
    free(includeSource);
    free(includeResult);
    return;
}

static void
handleMacroWithStringArg(Scanner* scanner, WkMenu* menu, Token* token)
{
    assert(scanner && menu && token);

    makeScannerCurrent(scanner);

    Token result = {0};
    scanTokenForPreprocessor(scanner, &result, SCANNER_WANTS_DESCRIPTION);
    if (result.type != TOKEN_DESCRIPTION)
    {
        errorAtToken(
            &result, scanner->filepath,
            "Expect string argument to '%.*s' macro. Got '%.*s'.",
            token->length, token->start,
            result.length, result.start
        );
        scanner->hadError = true;
    }

    String arg = {0};
    initString(&arg);
    appendToString(&arg, result.start, result.length);

    switch (token->type)
    {
    case TOKEN_FOREGROUND_COLOR:
    {
        setMenuColor(menu, arg.string, WK_COLOR_FOREGROUND);
        menu->garbage.foregroundColor = arg.string;
        break;
    }
    case TOKEN_BACKGROUND_COLOR:
    {
        setMenuColor(menu, arg.string, WK_COLOR_BACKGROUND);
        menu->garbage.backgroundColor = arg.string;
        break;
    }
    case TOKEN_BORDER_COLOR:
    {
        setMenuColor(menu, arg.string, WK_COLOR_BORDER);
        menu->garbage.borderColor = arg.string;
        break;
    }
    case TOKEN_SHELL: menu->shell = menu->garbage.shell = arg.string; break;
    case TOKEN_FONT: menu->font = menu->garbage.font = arg.string; break;
    case TOKEN_INCLUDE: break;
    default:
    {
        errorMsg(
            "Got an unexpected token to function `handleMacroWithStringArg`."
        );
        scanner->hadError = true;
        break;
    }
    }

    disownString(&arg);
}

static void
handleMacroWithDoubleArg(Scanner* scanner, WkMenu* menu, Token* token)
{
    assert(scanner && menu && token);

    makeScannerCurrent(scanner);

    Token result = {0};
    scanTokenForPreprocessor(scanner, &result, SCANNER_WANTS_DOUBLE);
    if (result.type != TOKEN_DOUBLE) goto fail;

    double value = 0;
    if (!getDoubleFromToken(&result, &value, menu->debug)) goto fail;

    switch (token->type)
    {
    case TOKEN_BORDER_RADIUS: menu->borderRadius = value; return;
    default:
    {
        errorMsg(
            "Got an unexpected token to function `handleSwitchWithNumberArg`."
        );
        scanner->hadError = true;
        return;
    }
    }

fail:
    errorAtToken(
        &result, scanner->filepath,
        "Expect double after '%.*s' switch. Got '%.*s'.",
        token->length, token->start,
        result.length, result.start
    );
    scanner->hadError = true;
    return;
}

static void
handleMacroWithInt32Arg(Scanner* scanner, WkMenu* menu, Token* token)
{
    assert(scanner && menu && token);

    makeScannerCurrent(scanner);

    Token result = {0};
    scanTokenForPreprocessor(scanner, &result, SCANNER_WANTS_INTEGER);
    if (result.type != TOKEN_INTEGER) goto fail;

    int32_t value = 0;
    if (!getInt32FromToken(&result, &value, menu->debug)) goto fail;

    switch (token->type)
    {
    case TOKEN_WINDOW_WIDTH: menu->windowWidth = value; return;
    case TOKEN_WINDOW_GAP: menu->windowGap = value; return;
    default:
    {
        errorMsg(
            "Got an unexpected token to function `handleSwitchWithInt32Arg`."
        );
        scanner->hadError = true;
        return;
    }
    }

fail:
    errorAtToken(
        &result, scanner->filepath,
        "Expect integer after '%.*s' switch. Got '%.*s'.",
        token->length, token->start,
        result.length, result.start
    );
    scanner->hadError = true;
    return;
}

static void
handleMacroWithUint32Arg(Scanner* scanner, WkMenu* menu, Token* token)
{
    assert(scanner && menu && token);

    makeScannerCurrent(scanner);

    Token result = {0};
    scanTokenForPreprocessor(scanner, &result, SCANNER_WANTS_UNSIGNED_INTEGER);
    if (result.type != TOKEN_UNSIGNED_INTEGER) goto fail;

    uint32_t value = 0;
    if (!getUint32FromToken(&result, &value, menu->debug)) goto fail;

    switch (token->type)
    {
    case TOKEN_MAX_COLUMNS: menu->maxCols = value; return;
    case TOKEN_BORDER_WIDTH: menu->borderWidth = value; return;
    case TOKEN_WIDTH_PADDING: menu->wpadding = value; return;
    case TOKEN_HEIGHT_PADDING: menu->hpadding = value; return;
    case TOKEN_BORDER_RADIUS: menu->borderRadius = value; return;
    default:
    {
        /* TODO error logic */
        errorMsg(
            "Got an unexpected token to function `handleSwitchWithUint32Arg`."
        );
        scanner->hadError = true;
        return;
    }
    }

fail:
    errorAtToken(
        &result, scanner->filepath,
        "Expect unsigned integer after '%.*s' switch. Got '%.*s'.",
        token->length, token->start,
        result.length, result.start
    );
    scanner->hadError = true;
    return;
}

char*
runPreprocessor(WkMenu* menu, const char* source, const char* filepath)
{
    assert(menu && source);

    Scanner scanner = {0};
    initScanner(&scanner, source, filepath);
    PieceTable pieceTable;
    initPieceTable(&pieceTable, source);
    const char* scannerStart = scanner.head;

    while (!isAtEnd(&scanner))
    {
        if (scanner.hadError) goto fail;

        Token token = {0};
        scanTokenForPreprocessor(&scanner, &token, SCANNER_WANTS_MACRO);
        if (token.type == TOKEN_EOF) break;
        if (menu->debug) disassembleSingleToken(&token);

        /* Found either valid preprocessor token, or error. Either way it is safe to append. */
        appendToPieceTable(
            &pieceTable, PIECE_SOURCE_ORIGINAL, scannerStart, scanner.start - 1 - scannerStart
        );

        /* Handle macros */
        switch (token.type)
        {
        /* Switches with no args. */
        case TOKEN_DEBUG: menu->debug = true; break;
        case TOKEN_TOP: menu->position = WK_WIN_POS_TOP; break;
        case TOKEN_BOTTOM: menu->position = WK_WIN_POS_BOTTOM; break;

        /* Switches with signed integer args. */
        case TOKEN_WINDOW_WIDTH:    /* FALLTHROUGH */
        case TOKEN_WINDOW_GAP: handleMacroWithInt32Arg(&scanner, menu, &token); break;

        /* Switches with unsigned integer args. */
        case TOKEN_MAX_COLUMNS:   /* FALLTHROUGH */
        case TOKEN_BORDER_WIDTH:
        case TOKEN_WIDTH_PADDING:
        case TOKEN_HEIGHT_PADDING: handleMacroWithUint32Arg(&scanner, menu, &token); break;

        /* Switches with double args. */
        case TOKEN_BORDER_RADIUS: handleMacroWithDoubleArg(&scanner, menu, &token); break;

        /* Switches with string args. */
        case TOKEN_FOREGROUND_COLOR: /* FALLTHROUGH */
        case TOKEN_BACKGROUND_COLOR:
        case TOKEN_BORDER_COLOR:
        case TOKEN_SHELL:
        case TOKEN_FONT: handleMacroWithStringArg(&scanner, menu, &token); break;
        case TOKEN_INCLUDE: handleIncludeMacro(menu, &scanner, &pieceTable); break;

        /* Handle error */
        case TOKEN_ERROR:
        {
            errorAtToken(
                &token, scanner.filepath,
                "%s", token.message
            );
            scanner.hadError = true;
            break;
        }
        default:
        {
            if (menu->debug)
            {
                /* DEBUG HERE */
            }
            errorAtToken(
                &token, scanner.filepath,
                "Got unexpected token during preprocessor parsing."
            );
            scanner.hadError = true;
            break;
        }
        }

        /* Update scanner for the next go around. */
        scannerStart = scanner.current;
    }

    /* Append the last bit of the source to result. */
    appendToPieceTable(
        &pieceTable, PIECE_SOURCE_ORIGINAL, scannerStart, scanner.current - scannerStart
    );

    if (menu->debug)
    {
        disassemblePieceTable(&pieceTable);
        disassembleMenu(menu);
    }
    char* result = compilePieceTableToString(&pieceTable);
    freePieceTable(&pieceTable);
    return result;

fail:
    freePieceTable(&pieceTable);
    return NULL;
}
