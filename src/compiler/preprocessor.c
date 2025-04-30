#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* common includes */
#include "common/arena.h"
#include "common/array.h"
#include "common/common.h"
#include "common/debug.h"
#include "common/menu.h"
#include "common/stack.h"

/* local includes */
#include "preprocessor.h"
#include "debug.h"
#include "scanner.h"
#include "token.h"

typedef struct
{
    char* path;
} FilePath;

static Array preprocessorRunImpl(Menu*, Array*, const char*, Stack*, Arena*);

static bool
fileIsInIncludeStack(Stack* stack, const char* filepath)
{
    assert(stack), assert(filepath);

    forEach(stack, FilePath, entry)
    {
        if (strcmp(entry->path, filepath) == 0) return true;
    }

    return false;
}

static void
disassembleIncludeStack(Stack* stack)
{
    assert(stack);

    char* pwd = getenv("PWD");
    if (!pwd)
    {
        warnMsg("Could not get environment variable '$PWD' for include stack debug output.");
        pwd = "";
    }
    size_t pwdLen = strlen(pwd);

    debugMsg(true, "");
    debugPrintHeader("IncludeStack");
    debugMsg(true, "|");
    forEach(stack, FilePath, entry)
    {
        if (strncmp(entry->path, pwd, pwdLen) == 0 && entry->path[pwdLen] == '/')
        {
            debugMsg(true, "| %4zu | %s", iter.index, entry->path + pwdLen + 1);
        }
        else
        {
            debugMsg(true, "| %4zu | %s", iter.index, entry->path);
        }
    }
    debugMsg(true, "|");
    debugPrintHeader("");
    debugMsg(true, "");
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
    Array includeFilePath = ARRAY_INIT(char);

    /* If filepath is not absolute and source path is not in the PWD add base path */
    if (*filepath != '/' && baseLen > 0) arrayAppendN(&includeFilePath, sourcePath, baseLen);

    arrayAppendN(&includeFilePath, filepath, len);
    arrayAppend(&includeFilePath, "");
    char* realPath = realpath(ARRAY_AS(&includeFilePath, char), NULL);
    if (!realPath)
    {
        warnMsg("Could not get the realpath for file: '%.*s'.", len, filepath);
        arrayFree(&includeFilePath);
        return NULL;
    }

    arrayFree(&includeFilePath);
    return realPath;
}

static void
handleIncludeMacro(
    Menu* menu,
    Scanner* scanner,
    Array* result,
    Token* includeFile,
    Stack* stack,
    Arena* arena
) {
    assert(menu), assert(scanner), assert(result), assert(includeFile), assert(stack), assert(arena);

    /* currently pointing at the 'i' in ':include', so take off one. */
    const char* sourcePath = scanner->filepath;

    if (!sourcePath) sourcePath = getenv("PWD");
    if (!sourcePath)
    {
        warnMsg("Could not get environment variable '$PWD' for script.");
    }

    char* includeFilePath = NULL;
    Array includeSource = ARRAY_INIT(char);
    Array includeResult = ARRAY_INIT(char);

    /* Get the path to the included file */
    includeFilePath = getAbsolutePath(
        includeFile->start, includeFile->length, sourcePath
    );
    if (!includeFilePath)
    {
        errorMsg("Failed to get the included file path.");
        scanner->hadError = true;
        return;
    }

    if (fileIsInIncludeStack(stack, includeFilePath))
    {
        printf(
            "%s:%u:%u: wk does not support circular includes: ':include \"%.*s\"'.\n",
            sourcePath, includeFile->line, includeFile->column,
            (int)includeFile->length, includeFile->start
        );
        if (menu->debug) disassembleIncludeStack(stack);
        scanner->hadError = true;
        goto fail;
    }

    /* Try to read the included file */
    includeSource = readFile(includeFilePath);
    if (arrayIsEmpty(&includeSource))
    {
        /* readFile prints an error for us. */
        scanner->hadError = true;
        goto fail;
    }

    /* check that the file and the source are not one and the same */
    if (strcmp(ARRAY_AS(&includeSource, char), scanner->head) == 0)
    {
        errorMsg(
            "Included file appears to be the same as the source file. Cannot `:include` self."
        );
        scanner->hadError = true;
        goto fail;
    }

    /* Run preprocessor on the included file */
    includeResult = preprocessorRunImpl(menu, &includeSource, includeFilePath, stack, arena);
    if (arrayIsEmpty(&includeResult))
    {
        errorMsg("Failed to get preprocessor result.");
        scanner->hadError = true;
        goto fail;
    }

    /* Append the result. */
    arrayAppendN(result, ARRAY_AS(&includeResult, char), arrayLength(&includeResult));
    /* if (!arrayIsEmpty(&includeResult)) */
    /* { */
    /*     arrayAppendN(result, ARRAY_AS(&includeResult, char), arrayLength(&includeResult) - 1); */
    /* } */

fail:
    free(includeFilePath);
    arrayFree(&includeSource);
    arrayFree(&includeResult);
    return;
}

static void
handleMacroWithStringArg(
    Menu* menu,
    Scanner* scanner,
    Token* token,
    Array* arr,
    Stack* stack,
    Arena* arena
) {
    assert(menu), assert(scanner), assert(token), assert(arr), assert(stack), assert(arena);

    scannerMakeCurrent(scanner);

    Token result = {0};
    scannerGetTokenForPreprocessor(scanner, &result, SCANNER_WANTS_DESCRIPTION);
    if (result.type != TOKEN_DESCRIPTION)
    {
        tokenErrorAt(
            token, scanner->filepath,
            "Expect string argument to macro. Got '%.*s'.",
            token->length, token->start,
            result.length, result.start
        );
        scanner->hadError = true;
        return;
    }

    char* arg = arenaCopyCString(arena, result.start, result.length);

    switch (token->type)
    {
    case TOKEN_FOREGROUND_COLOR:
    {
        menuSetColor(menu, arg, MENU_COLOR_KEY);
        menuSetColor(menu, arg, MENU_COLOR_DELIMITER);
        menuSetColor(menu, arg, MENU_COLOR_PREFIX);
        break;
    }
    case TOKEN_FOREGROUND_KEY_COLOR: menuSetColor(menu, arg, MENU_COLOR_KEY); break;
    case TOKEN_FOREGROUND_DELIMITER_COLOR: menuSetColor(menu, arg, MENU_COLOR_DELIMITER); break;
    case TOKEN_FOREGROUND_PREFIX_COLOR: menuSetColor(menu, arg, MENU_COLOR_PREFIX); break;
    case TOKEN_FOREGROUND_CHORD_COLOR: menuSetColor(menu, arg, MENU_COLOR_CHORD); break;
    case TOKEN_BACKGROUND_COLOR: menuSetColor(menu, arg, MENU_COLOR_BACKGROUND); break;
    case TOKEN_BORDER_COLOR: menuSetColor(menu, arg, MENU_COLOR_BORDER); break;
    case TOKEN_SHELL: menu->shell = arg; break;
    case TOKEN_FONT: menu->font = arg; break;
    case TOKEN_IMPLICIT_ARRAY_KEYS: menu->implicitArrayKeys = arg; break;
    case TOKEN_INCLUDE: handleIncludeMacro(menu, scanner, arr, &result, stack, arena); break;
    default:
    {
        errorMsg(
            "Got an unexpected token to function `handleMacroWithStringArg`."
        );
        scanner->hadError = true;
        break;
    }
    }
}

static void
handleMacroWithDoubleArg(Scanner* scanner, Menu* menu, Token* token)
{
    assert(scanner), assert(menu), assert(token);

    scannerMakeCurrent(scanner);

    Token result = {0};
    scannerGetTokenForPreprocessor(scanner, &result, SCANNER_WANTS_DOUBLE);
    if (result.type != TOKEN_DOUBLE) goto fail;

    double value = 0;
    if (!tokenGetDouble(&result, &value, menu->debug)) goto fail;

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
    tokenErrorAt(
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

    scannerMakeCurrent(scanner);

    Token result = {0};
    scannerGetTokenForPreprocessor(scanner, &result, SCANNER_WANTS_INTEGER);
    if (result.type != TOKEN_INTEGER) goto fail;

    int32_t value = 0;
    if (!tokenGetInt32(&result, &value, menu->debug)) goto fail;

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
    tokenErrorAt(
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

    scannerMakeCurrent(scanner);

    Token result = {0};
    scannerGetTokenForPreprocessor(scanner, &result, SCANNER_WANTS_UNSIGNED_INTEGER);
    if (result.type != TOKEN_UNSIGNED_INTEGER) goto fail;

    uint32_t value = 0;
    if (!tokenGetUint32(&result, &value, menu->debug)) goto fail;

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
    tokenErrorAt(
        token, scanner->filepath,
        "Expect unsigned integer argument to macro. Got '%.*s'.",
        token->length, token->start,
        result.length, result.start
    );
    scanner->hadError = true;
    return;
}

static void
popFilePath(Stack* stack)
{
    assert(stack);

    if (stackIsEmpty(stack)) return;

    FilePath* entry = STACK_PEEK(stack, FilePath);
    free(entry->path);
    stackPop(stack);
}

static Array
preprocessorRunImpl(Menu* menu, Array* source, const char* filepath, Stack* stack, Arena* arena)
{
    assert(menu), assert(source), assert(stack), assert(arena);

    Scanner scanner = {0};
    scannerInit(&scanner, ARRAY_AS(source, char), filepath);
    const char* scannerStart = scanner.head;

    Array result = ARRAY_INIT(char);

    char* absoluteFilePath = getAbsolutePath(filepath, strlen(filepath), ".");
    FilePath pathEntry = { .path = absoluteFilePath };
    stackPush(stack, &pathEntry);
    if (menu->debug)
    {
        disassembleIncludeStack(stack);
        disassembleArrayAsText(source, "Source");
    }

    while (!scannerIsAtEnd(&scanner))
    {
        if (scanner.hadError) goto fail;

        Token token = {0};
        scannerGetTokenForPreprocessor(&scanner, &token, SCANNER_WANTS_MACRO);
        if (token.type == TOKEN_EOF) break;
        if (menu->debug) disassembleSingleToken(&token);

        /* Found either valid preprocessor token, or error. Either way it is safe to append. */
        arrayAppendN(&result, scannerStart, scanner.start - 1 - scannerStart);

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
        case TOKEN_IMPLICIT_ARRAY_KEYS:
        case TOKEN_INCLUDE:
        {
            handleMacroWithStringArg(menu, &scanner, &token, &result, stack, arena);
            break;
        }

        /* Handle error */
        case TOKEN_ERROR:
        {
            tokenErrorAt(
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
            tokenErrorAt(
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
    arrayAppendN(&result, scannerStart, scanner.current - scannerStart);

    if (menu->debug)
    {
        disassembleArrayAsText(&result, "Processed Source");
        disassembleMenu(menu);
    }

fail:
    popFilePath(stack);

    if (scanner.hadError) arrayFree(&result);
    return result;
}

Array
preprocessorRun(Menu* menu, Array* source, const char* filepath)
{
    assert(menu), assert(source);

    Stack stack = STACK_INIT(FilePath);
    Array result = preprocessorRunImpl(menu, source, filepath, &stack, &menu->arena);
    while (!stackIsEmpty(&stack)) popFilePath(&stack);
    stackFree(&stack);
    arrayAppend(&result, "");
    return result;
}
