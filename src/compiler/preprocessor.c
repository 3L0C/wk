#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* common includes */
#include "common/arena.h"
#include "common/common.h"
#include "common/debug.h"
#include "common/menu.h"
#include "common/stack.h"
#include "common/string.h"

/* local includes */
#include "debug.h"
#include "lazy_string.h"
#include "preprocessor.h"
#include "scanner.h"
#include "token.h"

typedef struct
{
    char* path;
    char* canonicalPath;
} FilePath;

static String preprocessorRunImpl(Menu*, String, const char*, Stack*, Arena*);

static bool
fileIsInIncludeStack(Stack* stack, const char* canonicalPath)
{
    assert(stack), assert(canonicalPath);

    vectorForEach(stack, FilePath, entry)
    {
        if (entry->canonicalPath && strcmp(entry->canonicalPath, canonicalPath) == 0) return true;
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
    vectorForEach(stack, FilePath, entry)
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

    size_t baseLen         = getBaseDirLength(sourcePath);
    Vector includeFilePath = VECTOR_INIT(char);

    if (*filepath != '/' && baseLen > 0) vectorAppendN(&includeFilePath, sourcePath, baseLen);

    vectorAppendN(&includeFilePath, filepath, len);
    vectorAppend(&includeFilePath, "");

    char* constructedPath = VECTOR_AS(&includeFilePath, char);

    FILE* f = fopen(constructedPath, "r");
    if (!f)
    {
        warnMsg("Could not open file: '%.*s'.", len, filepath);
        vectorFree(&includeFilePath);
        return NULL;
    }
    fclose(f);

    char* result = strdup(constructedPath);
    vectorFree(&includeFilePath);
    return result;
}

static char*
getArg(Menu* menu, Scanner* scanner, Arena* arena, Token* firstToken, const char* context)
{
    assert(menu), assert(scanner), assert(arena), assert(firstToken), assert(context);

    /* If first token is TOKEN_DESCRIPTION, there are no interpolations - return literal string */
    if (firstToken->type == TOKEN_DESCRIPTION)
    {
        LazyString result = lazyStringInit();
        lazyStringAppendEscString(&result, firstToken->start, firstToken->length);
        char* resolved = lazyStringToCString(arena, &result);
        lazyStringFree(&result);
        return resolved;
    }

    LazyString result = lazyStringInit();
    Token      token  = { 0 };
    tokenCopy(firstToken, &token);

    while (token.type != TOKEN_DESCRIPTION)
    {
        switch (token.type)
        {
        case TOKEN_DESC_INTERP:
        {
            /* Literal text between interpolations */
            lazyStringAppendEscString(&result, token.start, token.length);
            break;
        }
        case TOKEN_USER_VAR:
        {
            /* Look up variable and substitute */
            bool found = false;
            vectorForEach(&menu->userVars, const UserVar, var)
            {
                if (strncmp(var->key, token.start, token.length) == 0 &&
                    strlen(var->key) == token.length)
                {
                    lazyStringAppendCString(&result, var->value);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                scannerErrorAt(
                    scanner,
                    &token,
                    "Undefined variable '%.*s' in %s. "
                    "Variables must be defined with :var before use.",
                    (int)token.length,
                    token.start,
                    context);
                scanner->hadError = true;
                lazyStringFree(&result);
                return NULL;
            }
            break;
        }
        default:
        {
            scannerErrorAt(
                scanner,
                &token,
                "Unexpected token type in %s interpolation.",
                context);
            scanner->hadError = true;
            lazyStringFree(&result);
            return NULL;
        }
        }

        scannerTokenForPreprocessor(scanner, &token, SCANNER_WANTS_DESCRIPTION);
    }

    /* TOKEN_DESCRIPTION can have trailing literal text after the last interpolation */
    if (token.length > 0)
    {
        lazyStringAppendEscString(&result, token.start, token.length);
    }

    char* resolved = lazyStringToCString(arena, &result);
    lazyStringFree(&result);
    return resolved;
}

static void
handleIncludeMacro(
    Menu*       menu,
    Scanner*    scanner,
    LazyString* result,
    Token*      includeFile,
    Stack*      stack,
    Arena*      arena)
{
    assert(menu), assert(scanner), assert(result), assert(includeFile), assert(stack),
        assert(arena);

    /* currently pointing at the 'i' in ':include', so take off one. */
    const char* sourcePath = scanner->filepath;

    if (!sourcePath) sourcePath = getenv("PWD");
    if (!sourcePath)
    {
        warnMsg("Could not get environment variable '$PWD' for script.");
    }

    char* includeFilePath = NULL;

    /* Get the path to the included file */
    includeFilePath = getAbsolutePath(includeFile->start, includeFile->length, sourcePath);
    if (!includeFilePath)
    {
        errorMsg("Failed to get the included file path.");
        scanner->hadError = true;
        return;
    }

    /* Resolve canonical path for circular include detection */
    char* canonicalIncludePath = realpath(includeFilePath, NULL);
    if (!canonicalIncludePath)
    {
        warnMsg("Could not get the canonical path for file: '%s'.", includeFilePath);
        /* Continue with the constructed path if canonicalization fails */
        canonicalIncludePath = strdup(includeFilePath);
    }

    if (fileIsInIncludeStack(stack, canonicalIncludePath))
    {
        printf(
            "%s:%u:%u: wk does not support circular includes: ':include \"%.*s\"'.\n",
            sourcePath,
            includeFile->line,
            includeFile->column,
            (int)includeFile->length,
            includeFile->start);
        if (menu->debug) disassembleIncludeStack(stack);
        scanner->hadError = true;
        free(canonicalIncludePath);
        free(includeFilePath);
        return;
    }

    free(canonicalIncludePath);

    /* Try to read the included file into arena */
    String includeSource = readFileToArena(arena, includeFilePath);
    if (stringIsEmpty(&includeSource))
    {
        /* readFileToArena prints an error for us. */
        scanner->hadError = true;
        free(includeFilePath);
        return;
    }

    /* check that the file and the source are not one and the same */
    if (strcmp(includeSource.data, scanner->head) == 0)
    {
        errorMsg(
            "Included file appears to be the same as the source file. Cannot `:include` self.");
        scanner->hadError = true;
        free(includeFilePath);
        return;
    }

    /* Run preprocessor on the included file */
    String includeResult = preprocessorRunImpl(menu, includeSource, includeFilePath, stack, arena);
    if (stringIsEmpty(&includeResult))
    {
        errorMsg("Failed to get preprocessor result.");
        scanner->hadError = true;
        free(includeFilePath);
        return;
    }

    /* Append the result using LazyString lazy concat. */
    lazyStringAppend(result, includeResult.data, includeResult.length);

    free(includeFilePath);
}

static void
handleVarMacro(Menu* menu, Scanner* scanner, Token* keyToken, char* key, Arena* arena)
{
    assert(menu), assert(scanner), assert(keyToken), assert(key), assert(arena);

    /* Resolve variables in variable NAME (meta-variables) */
    char* resolvedKey = getArg(menu, scanner, arena, keyToken, "variable name");
    if (!resolvedKey) return; /* Error already reported */

    /* Validate resolved name */
    if (strlen(resolvedKey) == 0)
    {
        scannerErrorAt(scanner, keyToken, "Variable name resolves to empty string.");
        scanner->hadError = true;
        return;
    }

    if (strchr(resolvedKey, ')'))
    {
        scannerErrorAt(
            scanner,
            keyToken,
            "Variable name contains ')' after resolution: '%s'.",
            resolvedKey);
        scanner->hadError = true;
        return;
    }

    scannerMakeCurrent(scanner);

    Token valueToken = { 0 };
    scannerTokenForPreprocessor(scanner, &valueToken, SCANNER_WANTS_DESCRIPTION);

    if (valueToken.type != TOKEN_DESCRIPTION && valueToken.type != TOKEN_DESC_INTERP &&
        valueToken.type != TOKEN_USER_VAR)
    {
        scannerErrorAt(
            scanner,
            &valueToken,
            "Expected variable value as a string. Got '%.*s'.",
            valueToken.length,
            valueToken.start);
        scanner->hadError = true;
        return;
    }

    /* Resolve variables in variable VALUE (variables referencing variables) */
    char* resolvedValue = getArg(menu, scanner, arena, &valueToken, "variable value");
    if (!resolvedValue) return; /* Error already reported */

    vectorForEach(&menu->userVars, UserVar, var)
    {
        if (strcmp(var->key, resolvedKey) == 0)
        {
            var->key   = resolvedKey;
            var->value = resolvedValue;
            return;
        }
    }

    UserVar newVar = { .key = resolvedKey, .value = resolvedValue };
    vectorAppend(&menu->userVars, &newVar);
}

static void
handleMacroWithStringArg(
    Menu*       menu,
    Scanner*    scanner,
    Token*      token,
    LazyString* output,
    Stack*      stack,
    Arena*      arena)
{
    assert(menu), assert(scanner), assert(token), assert(output), assert(stack), assert(arena);

    scannerMakeCurrent(scanner);

    Token argToken = { 0 };
    scannerTokenForPreprocessor(scanner, &argToken, SCANNER_WANTS_DESCRIPTION);
    if (argToken.type != TOKEN_DESCRIPTION && argToken.type != TOKEN_DESC_INTERP &&
        argToken.type != TOKEN_USER_VAR)
    {
        scannerErrorAt(
            scanner,
            token,
            "Expect string argument to macro. Got '%.*s'.",
            token->length,
            token->start,
            argToken.length,
            argToken.start);
        scanner->hadError = true;
        return;
    }

    switch (token->type)
    {
    case TOKEN_INCLUDE:
    {
        handleIncludeMacro(menu, scanner, output, &argToken, stack, arena);
        break;
    }
    case TOKEN_VAR:
    {
        char* key = arenaCopyCString(arena, argToken.start, argToken.length);
        handleVarMacro(menu, scanner, &argToken, key, arena);
        break;
    }
    case TOKEN_FOREGROUND_COLOR: /* FALLTHROUGH */
    case TOKEN_FOREGROUND_KEY_COLOR:
    case TOKEN_FOREGROUND_DELIMITER_COLOR:
    case TOKEN_FOREGROUND_PREFIX_COLOR:
    case TOKEN_FOREGROUND_CHORD_COLOR:
    case TOKEN_FOREGROUND_TITLE_COLOR:
    case TOKEN_BACKGROUND_COLOR:
    case TOKEN_BORDER_COLOR:
    case TOKEN_SHELL:
    case TOKEN_FONT:
    case TOKEN_MENU_TITLE:
    case TOKEN_MENU_TITLE_FONT:
    case TOKEN_IMPLICIT_ARRAY_KEYS:
    case TOKEN_WRAP_CMD:
    case TOKEN_DELIMITER:
    {
        /* Menu settings must persist past compilation, so use menu->arena not compilerArena */
        char* arg = getArg(menu, scanner, &menu->arena, &argToken, "macro argument");
        if (!arg) return; /* Error already reported */

        switch (token->type)
        {
        case TOKEN_FOREGROUND_COLOR:
            menuSetColor(menu, arg, MENU_COLOR_KEY);
            menuSetColor(menu, arg, MENU_COLOR_DELIMITER);
            menuSetColor(menu, arg, MENU_COLOR_PREFIX);
            menuSetColor(menu, arg, MENU_COLOR_CHORD);
            menuSetColor(menu, arg, MENU_COLOR_TITLE);
            break;
        case TOKEN_FOREGROUND_KEY_COLOR: menuSetColor(menu, arg, MENU_COLOR_KEY); break;
        case TOKEN_FOREGROUND_DELIMITER_COLOR: menuSetColor(menu, arg, MENU_COLOR_DELIMITER); break;
        case TOKEN_FOREGROUND_PREFIX_COLOR: menuSetColor(menu, arg, MENU_COLOR_PREFIX); break;
        case TOKEN_FOREGROUND_CHORD_COLOR: menuSetColor(menu, arg, MENU_COLOR_CHORD); break;
        case TOKEN_FOREGROUND_TITLE_COLOR: menuSetColor(menu, arg, MENU_COLOR_TITLE); break;
        case TOKEN_BACKGROUND_COLOR: menuSetColor(menu, arg, MENU_COLOR_BACKGROUND); break;
        case TOKEN_BORDER_COLOR: menuSetColor(menu, arg, MENU_COLOR_BORDER); break;
        case TOKEN_SHELL: menu->shell = arg; break;
        case TOKEN_FONT: menu->font = arg; break;
        case TOKEN_MENU_TITLE: menu->title = menu->rootTitle = arg; break;
        case TOKEN_MENU_TITLE_FONT: menu->titleFont = arg; break;
        case TOKEN_IMPLICIT_ARRAY_KEYS: menu->implicitArrayKeys = arg; break;
        case TOKEN_WRAP_CMD: menuSetWrapCmd(menu, arg); break;
        case TOKEN_DELIMITER: menu->delimiter = arg; break;
        default: break;
        }
        break;
    }
    default:
    {
        errorMsg("Got an unexpected token to function `handleMacroWithStringArg`.");
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

    Token result = { 0 };
    scannerTokenForPreprocessor(scanner, &result, SCANNER_WANTS_DOUBLE);
    if (result.type != TOKEN_DOUBLE) goto fail;

    double value = 0;
    if (!tokenDouble(&result, &value, menu->debug)) goto fail;

    switch (token->type)
    {
    case TOKEN_BORDER_RADIUS: menu->borderRadius = value; return;
    default:
    {
        errorMsg("Got an unexpected token to function `handleSwitchWithNumberArg`.");
        scanner->hadError = true;
        return;
    }
    }

fail:
    scannerErrorAt(
        scanner,
        token,
        "Expect double argument to macro. Got '%.*s'.",
        token->length,
        token->start,
        result.length,
        result.start);
    scanner->hadError = true;
    return;
}

static void
handleMacroWithInt32Arg(Scanner* scanner, Menu* menu, Token* token)
{
    assert(scanner), assert(menu), assert(token);

    scannerMakeCurrent(scanner);

    Token result = { 0 };
    scannerTokenForPreprocessor(scanner, &result, SCANNER_WANTS_INTEGER);
    if (result.type != TOKEN_INTEGER) goto fail;

    int32_t value = 0;
    if (!tokenInt32(&result, &value, menu->debug)) goto fail;

    switch (token->type)
    {
    case TOKEN_MENU_WIDTH: menu->menuWidth = value; return;
    case TOKEN_MENU_GAP: menu->menuGap = value; return;
    case TOKEN_TABLE_PADDING: menu->tablePadding = value; return;
    default:
    {
        errorMsg("Got an unexpected token to function `handleSwitchWithInt32Arg`.");
        scanner->hadError = true;
        return;
    }
    }

fail:
    scannerErrorAt(
        scanner,
        token,
        "Expect integer argument to macro. Got '%.*s'.",
        token->length,
        token->start,
        result.length,
        result.start);
    scanner->hadError = true;
    return;
}

static void
handleMacroWithUint32Arg(Scanner* scanner, Menu* menu, Token* token)
{
    assert(scanner), assert(menu), assert(token);

    scannerMakeCurrent(scanner);

    Token result = { 0 };
    scannerTokenForPreprocessor(scanner, &result, SCANNER_WANTS_UNSIGNED_INTEGER);
    if (result.type != TOKEN_UNSIGNED_INTEGER) goto fail;

    uint32_t value = 0;
    if (!tokenUint32(&result, &value, menu->debug)) goto fail;

    switch (token->type)
    {
    case TOKEN_MAX_COLUMNS: menu->maxCols = value; return;
    case TOKEN_BORDER_WIDTH: menu->borderWidth = value; return;
    case TOKEN_WIDTH_PADDING: menu->wpadding = value; return;
    case TOKEN_HEIGHT_PADDING: menu->hpadding = value; return;
    case TOKEN_MENU_DELAY: menu->delay = value; return;
    case TOKEN_KEEP_DELAY: menu->keepDelay = value; return;
    default:
    {
        errorMsg("Got an unexpected token to function `handleSwitchWithUint32Arg`.");
        scanner->hadError = true;
        return;
    }
    }

fail:
    scannerErrorAt(
        scanner,
        token,
        "Expect unsigned integer argument to macro. Got '%.*s'.",
        token->length,
        token->start,
        result.length,
        result.start);
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
    free(entry->canonicalPath);
    stackPop(stack);
}

static String
preprocessorRunImpl(Menu* menu, String source, const char* filepath, Stack* stack, Arena* arena)
{
    assert(menu), assert(stack), assert(arena);

    String emptyResult = { 0 };

    if (stringIsEmpty(&source)) return emptyResult;

    Scanner scanner = { 0 };
    scannerInit(&scanner, source.data, filepath);
    const char* scannerStart = scanner.head;

    LazyString result = lazyStringInit();

    /* For stdin, skip file path resolution - there's no real file */
    char* absoluteFilePath  = NULL;
    char* canonicalFilePath = NULL;
    if (strcmp(filepath, "<stdin>") == 0)
    {
        absoluteFilePath  = strdup(filepath);
        canonicalFilePath = strdup(filepath);
    }
    else
    {
        absoluteFilePath  = getAbsolutePath(filepath, strlen(filepath), ".");
        canonicalFilePath = realpath(absoluteFilePath ? absoluteFilePath : filepath, NULL);
        if (!canonicalFilePath)
        {
            /* If canonicalization fails, use the constructed path */
            canonicalFilePath = strdup(absoluteFilePath ? absoluteFilePath : filepath);
        }
    }

    FilePath pathEntry = { .path = absoluteFilePath, .canonicalPath = canonicalFilePath };
    stackPush(stack, &pathEntry);
    if (menu->debug)
    {
        disassembleIncludeStack(stack);
        debugPrintHeader("Source");
        debugTextLenWithLineNumber(source.data, source.length);
        debugPrintHeader("");
    }

    while (!scannerIsAtEnd(&scanner))
    {
        if (scanner.hadError) goto fail;

        Token token = { 0 };
        scannerTokenForPreprocessor(&scanner, &token, SCANNER_WANTS_MACRO);
        if (token.type == TOKEN_EOF) break;
        if (menu->debug) disassembleSingleToken(&token);

        /* Found either valid preprocessor token, or error. Either way it is safe to append. */
        lazyStringAppend(&result, scannerStart, scanner.start - 1 - scannerStart);

        /* Handle macros */
        switch (token.type)
        {
        /* Switches with no args. */
        case TOKEN_DEBUG: menu->debug = true; break;
        case TOKEN_UNSORTED: menu->sort = false; break;
        case TOKEN_TOP: menu->position = MENU_POS_TOP; break;
        case TOKEN_BOTTOM: menu->position = MENU_POS_BOTTOM; break;

        /* Switches with signed integer args. */
        case TOKEN_MENU_WIDTH: /* FALLTHROUGH */
        case TOKEN_MENU_GAP:
        case TOKEN_TABLE_PADDING: handleMacroWithInt32Arg(&scanner, menu, &token); break;

        /* Switches with unsigned integer args. */
        case TOKEN_MAX_COLUMNS: /* FALLTHROUGH */
        case TOKEN_BORDER_WIDTH:
        case TOKEN_WIDTH_PADDING:
        case TOKEN_HEIGHT_PADDING:
        case TOKEN_MENU_DELAY:
        case TOKEN_KEEP_DELAY: handleMacroWithUint32Arg(&scanner, menu, &token); break;

        /* Switches with double args. */
        case TOKEN_BORDER_RADIUS: handleMacroWithDoubleArg(&scanner, menu, &token); break;

        /* Switches with string args. */
        case TOKEN_FOREGROUND_COLOR: /* FALLTHROUGH */
        case TOKEN_FOREGROUND_KEY_COLOR:
        case TOKEN_FOREGROUND_DELIMITER_COLOR:
        case TOKEN_FOREGROUND_PREFIX_COLOR:
        case TOKEN_FOREGROUND_CHORD_COLOR:
        case TOKEN_FOREGROUND_TITLE_COLOR:
        case TOKEN_BACKGROUND_COLOR:
        case TOKEN_BORDER_COLOR:
        case TOKEN_SHELL:
        case TOKEN_FONT:
        case TOKEN_MENU_TITLE:
        case TOKEN_MENU_TITLE_FONT:
        case TOKEN_IMPLICIT_ARRAY_KEYS:
        case TOKEN_WRAP_CMD:
        case TOKEN_DELIMITER:
        case TOKEN_INCLUDE:
        case TOKEN_VAR:
        {
            handleMacroWithStringArg(menu, &scanner, &token, &result, stack, arena);
            break;
        }

        /* Handle error */
        case TOKEN_ERROR:
        {
            scannerErrorAt(&scanner, &token, "%s", token.message);
            scanner.hadError = true;
            break;
        }
        default:
        {
            if (menu->debug)
            {
                /* DEBUG HERE */
            }
            scannerErrorAt(
                &scanner,
                &token,
                "Got unexpected token during preprocessor parsing.");
            scanner.hadError = true;
            break;
        }
        }

        /* Update scanner for the next go around. */
        scannerStart = scanner.current;
    }

    /* Append the last bit of the source to result. */
    lazyStringAppend(&result, scannerStart, scanner.current - scannerStart);

    if (menu->debug)
    {
        disassembleLazyString(&result, "Processed Source", 0);
        disassembleMenu(menu);
    }

fail:
    popFilePath(stack);

    if (scanner.hadError)
    {
        lazyStringFree(&result);
        return emptyResult;
    }

    String str = lazyStringToString(arena, &result);
    lazyStringFree(&result);
    return str;
}

String
preprocessorRun(Menu* menu, String source, const char* filepath, Arena* arena)
{
    assert(menu), assert(arena);

    Stack  stack  = STACK_INIT(FilePath);
    String result = preprocessorRunImpl(menu, source, filepath, &stack, arena);
    while (!stackIsEmpty(&stack))
    {
        popFilePath(&stack);
    }
    stackFree(&stack);
    return result;
}
