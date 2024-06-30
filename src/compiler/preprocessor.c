#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* common includes */
#include "common/common.h"
#include "common/debug.h"
#include "common/memory.h"
#include "common/menu.h"
#include "common/string.h"

/* local includes */
#include "preprocessor.h"
#include "debug.h"
#include "piece_table.h"
#include "scanner.h"
#include "token.h"

typedef struct
{
    const char** files;
    size_t count;
    size_t capacity;
} IncludeStack;

static IncludeStack stack = {0};

static void
initIncludeStack(IncludeStack* stack)
{
    assert(stack);

    stack->files = NULL;
    stack->count = 0;
    stack->capacity = 0;
}

static void
freeIncludeStack(IncludeStack* stack)
{
    assert(stack);

    FREE_ARRAY(const char*, stack->files, stack->capacity);
    initIncludeStack(stack);
}

static void
pushIncludeStack(IncludeStack* stack, const char* filepath)
{
    assert(stack), assert(filepath);

    if (stack->count == stack->capacity)
    {
        size_t oldCapacity = stack->capacity;
        stack->capacity = GROW_CAPACITY(oldCapacity);
        stack->files = GROW_ARRAY(const char*, stack->files, oldCapacity, stack->capacity);
    }

    stack->files[stack->count++] = filepath;
}

static void
popIncludeStack(IncludeStack* stack)
{
    assert(stack);
    if (stack->count == 0) return;

    stack->count--;
}

static bool
isFileInIncludeStack(IncludeStack* stack, const char* filepath)
{
    assert(stack), assert(filepath);

    for (size_t i = 0; i < stack->count; i++)
    {
        if (strcmp(stack->files[i], filepath) == 0) return true;
    }

    return false;
}

static void
disassembleIncludeStack(IncludeStack* stack)
{
    assert(stack);

    char* pwd = getenv("PWD");
    if (!pwd)
    {
        warnMsg("Could not get environment variable '$PWD' for include stack debug output.");
        pwd = "";
    }
    size_t pwdLen = strlen(pwd);

    debugPrintHeader(" IncludeStack ");
    debugMsg(true, "|");
    for (size_t i = 0; i < stack->count; i++)
    {
        const char* file = stack->files[i];
        if (strncmp(file, pwd, pwdLen) == 0 && file[pwdLen] == '/')
        {
            /* File path starts with PWD, display relative path */
            debugMsg(true, "| %4zu | %s", i, file + pwdLen + 1);
        }
        else
        {
            /* File path is not relative to PWD, display absolute path */
            debugMsg(true, "| %4zu | %s", i, file);
        }
    }
    debugMsg(true, "|");
    debugPrintHeader("");
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
getAbsolutePath(const char* filepath, size_t len, const char* sourcePath)
{
    assert(filepath), assert(sourcePath);

    size_t baseLen = getBaseDirLength(sourcePath);
    String includeFilePath = {0};
    initString(&includeFilePath);

    /* If filepath is not absolute and source path is not in the PWD add base path */
    if (*filepath != '/' && baseLen) appendToString(&includeFilePath, sourcePath, baseLen);

    appendToString(&includeFilePath, filepath, len);
    char* result = realpath(includeFilePath.string, NULL);
    if (!result)
    {
        warnMsg("Could not get the realpath for file: '%.*s'.", len, filepath);
        disownString(&includeFilePath);
        return includeFilePath.string;
    }

    freeString(&includeFilePath);
    return result;
}

static void
handleIncludeMacro(Menu* menu, Scanner* scanner, PieceTable* result, Token* includeFile)
{
    assert(menu), assert(scanner), assert(result), assert(includeFile);

    /* currently pointing at the 'i' in ':include', so take off one. */
    const char* sourcePath = scanner->filepath;

    if (!sourcePath) sourcePath = getenv("PWD");
    if (!sourcePath)
    {
        warnMsg("Could not get environment variable '$PWD' for script.");
    }

    char* includeFilePath = NULL;
    char* includeSource = NULL;
    char* includeResult = NULL;

    /* Get the path to the included file */
    includeFilePath = getAbsolutePath(
        includeFile->start, includeFile->length, sourcePath
    );
    if (!includeFilePath)
    {
        errorMsg("Failed to get the included file path.");
        scanner->hadError = true;
        goto fail;
    }

    if (isFileInIncludeStack(&stack, includeFilePath))
    {
        printf(
            "%s:%u:%u: wk does not support circular includes: ':include \"%.*s\"'.\n",
            sourcePath, includeFile->line, includeFile->column,
            (int)includeFile->length, includeFile->start
        );
        if (menu->debug) disassembleIncludeStack(&stack);
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
handleMacroWithStringArg(Menu* menu, Scanner* scanner, Token* token, PieceTable* pieceTable)
{
    assert(scanner), assert(menu), assert(token), assert(pieceTable);

    makeScannerCurrent(scanner);

    Token result = {0};
    scanTokenForPreprocessor(scanner, &result, SCANNER_WANTS_DESCRIPTION);
    if (result.type != TOKEN_DESCRIPTION)
    {
        errorAtToken(
            token, scanner->filepath,
            "Expect string argument to macro. Got '%.*s'.",
            token->length, token->start,
            result.length, result.start
        );
        scanner->hadError = true;
        return;
    }

    String arg = {0};
    initString(&arg);
    appendToString(&arg, result.start, result.length);

    switch (token->type)
    {
    case TOKEN_FOREGROUND_COLOR:
    {
        setMenuColor(menu, arg.string, MENU_COLOR_KEY);
        setMenuColor(menu, arg.string, MENU_COLOR_DELIMITER);
        setMenuColor(menu, arg.string, MENU_COLOR_PREFIX);
        menu->garbage.foregroundKeyColor = arg.string;
        break;
    }
    case TOKEN_FOREGROUND_KEY_COLOR:
    {
        setMenuColor(menu, arg.string, MENU_COLOR_KEY);
        menu->garbage.foregroundKeyColor = arg.string;
        break;
    }
    case TOKEN_FOREGROUND_DELIMITER_COLOR:
    {
        setMenuColor(menu, arg.string, MENU_COLOR_DELIMITER);
        menu->garbage.foregroundDelimiterColor= arg.string;
        break;
    }
    case TOKEN_FOREGROUND_PREFIX_COLOR:
    {
        setMenuColor(menu, arg.string, MENU_COLOR_PREFIX);
        menu->garbage.foregroundPrefixColor = arg.string;
        break;
    }
    case TOKEN_FOREGROUND_CHORD_COLOR:
    {
        setMenuColor(menu, arg.string, MENU_COLOR_CHORD);
        menu->garbage.foregroundChordColor = arg.string;
        break;
    }
    case TOKEN_BACKGROUND_COLOR:
    {
        setMenuColor(menu, arg.string, MENU_COLOR_BACKGROUND);
        menu->garbage.backgroundColor = arg.string;
        break;
    }
    case TOKEN_BORDER_COLOR:
    {
        setMenuColor(menu, arg.string, MENU_COLOR_BORDER);
        menu->garbage.borderColor = arg.string;
        break;
    }
    case TOKEN_SHELL: menu->shell = menu->garbage.shell = arg.string; break;
    case TOKEN_FONT: menu->font = menu->garbage.font = arg.string; break;
    case TOKEN_INCLUDE:
    {
        freeString(&arg);
        handleIncludeMacro(menu, scanner, pieceTable, &result);
        break;
    }
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
handleMacroWithDoubleArg(Scanner* scanner, Menu* menu, Token* token)
{
    assert(scanner), assert(menu), assert(token);

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
        token, scanner->filepath,
        "Expect double argument to macro. Got '%.*s'.",
        token->length, token->start,
        result.length, result.start
    );
    scanner->hadError = true;
    return;
}

static void
handleMacroWithInt32Arg(Scanner* scanner, Menu* menu, Token* token)
{
    assert(scanner), assert(menu), assert(token);

    makeScannerCurrent(scanner);

    Token result = {0};
    scanTokenForPreprocessor(scanner, &result, SCANNER_WANTS_INTEGER);
    if (result.type != TOKEN_INTEGER) goto fail;

    int32_t value = 0;
    if (!getInt32FromToken(&result, &value, menu->debug)) goto fail;

    switch (token->type)
    {
    case TOKEN_MENU_WIDTH: menu->menuWidth = value; return;
    case TOKEN_MENU_GAP: menu->menuGap = value; return;
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
        token, scanner->filepath,
        "Expect integer argument to macro. Got '%.*s'.",
        token->length, token->start,
        result.length, result.start
    );
    scanner->hadError = true;
    return;
}

static void
handleMacroWithUint32Arg(Scanner* scanner, Menu* menu, Token* token)
{
    assert(scanner), assert(menu), assert(token);

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
    case TOKEN_MENU_DELAY: menu->delay = value; return;
    default:
    {
        errorMsg(
            "Got an unexpected token to function `handleSwitchWithUint32Arg`."
        );
        scanner->hadError = true;
        return;
    }
    }

fail:
    errorAtToken(
        token, scanner->filepath,
        "Expect unsigned integer argument to macro. Got '%.*s'.",
        token->length, token->start,
        result.length, result.start
    );
    scanner->hadError = true;
    return;
}

char*
runPreprocessor(Menu* menu, const char* source, const char* filepath)
{
    assert(menu), assert(source);

    Scanner scanner = {0};
    initScanner(&scanner, source, filepath);
    const char* scannerStart = scanner.head;

    PieceTable pieceTable;
    initPieceTable(&pieceTable, source);

    /* Only init once. */
    if (stack.count == 0) initIncludeStack(&stack);
    char* absoluteFilePath = getAbsolutePath(filepath, strlen(filepath), ".");
    pushIncludeStack(&stack, absoluteFilePath);
    if (menu->debug) disassembleIncludeStack(&stack);

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
        case TOKEN_SORT: menu->sort = true; break;
        case TOKEN_TOP: menu->position = MENU_POS_TOP; break;
        case TOKEN_BOTTOM: menu->position = MENU_POS_BOTTOM; break;

        /* Switches with signed integer args. */
        case TOKEN_MENU_WIDTH: /* FALLTHROUGH */
        case TOKEN_MENU_GAP: handleMacroWithInt32Arg(&scanner, menu, &token); break;

        /* Switches with unsigned integer args. */
        case TOKEN_MAX_COLUMNS: /* FALLTHROUGH */
        case TOKEN_BORDER_WIDTH:
        case TOKEN_WIDTH_PADDING:
        case TOKEN_HEIGHT_PADDING:
        case TOKEN_MENU_DELAY: handleMacroWithUint32Arg(&scanner, menu, &token); break;

        /* Switches with double args. */
        case TOKEN_BORDER_RADIUS: handleMacroWithDoubleArg(&scanner, menu, &token); break;

        /* Switches with string args. */
        case TOKEN_FOREGROUND_COLOR: /* FALLTHROUGH */
        case TOKEN_FOREGROUND_KEY_COLOR:
        case TOKEN_FOREGROUND_DELIMITER_COLOR:
        case TOKEN_FOREGROUND_PREFIX_COLOR:
        case TOKEN_FOREGROUND_CHORD_COLOR:
        case TOKEN_BACKGROUND_COLOR:
        case TOKEN_BORDER_COLOR:
        case TOKEN_SHELL:
        case TOKEN_FONT:
        case TOKEN_INCLUDE: handleMacroWithStringArg(menu, &scanner, &token, &pieceTable); break;

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

fail:
    popIncludeStack(&stack);
    if (stack.count == 0) freeIncludeStack(&stack);
    free(absoluteFilePath);
    freePieceTable(&pieceTable);
    return scanner.hadError ? NULL : result;
}
