#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* common includes */
#include "common/common.h"
#include "common/memory.h"
#include "common/string.h"

/* local includes */
#include "preprocessor.h"
#include "scanner.h"

static bool hadError = false;
static bool debug = false;

static bool
isValidInclude(Scanner* scanner, const char** includeStart, const char** includeFileStart)
{
    assert(scanner && includeStart);

    /* `:include`s may only come at the start of a file, or after a new line. */
    /* if (scanner->head != scanner->current && advanceScanner(scanner) != '\n') return false; */

    /* Skip any comments or whitespace or such as
     * there may be space between the new line and the
     * `:include` instruction, which is not an issue. */
    skipWhitespace(scanner);

    /* Possibly at start of a keyword, possibly `:include`. Set start point. */
    *includeStart = scanner->current;

    /* If not at the start of an include directive, return. */
    if (!matchScanner(scanner, ':'))
    {
        advanceScanner(scanner);
        return false;
    }

    /* Get the type of identifier, may be a typical hook or flag. */
    makeScannerCurrent(scanner);
    Token token = getKeyword(scanner);
    if (token.type != TOKEN_INCLUDE) return false;

    /* Get the first '"' after the `include` */
    while (!isAtEnd(scanner) && peek(scanner) != '"') advanceScanner(scanner);

    /* Got eof or missing beginning '"' */
    if (!matchScanner(scanner, '"'))
    {
        errorMsg("Expect '\"' after `:include` preprocessor directive.");
        goto fail;
    }

    /* Set `includeFileStart` to begining of potential filename */
    *includeFileStart = scanner->current;

    /* Find closing '"' */
    while (!isAtEnd(scanner) && peek(scanner) != '"')
    {
        /* User's can escape double quotes in file names.
         * NOTE it is unlikely that a filename ends in a '\' character
         * but in the case where it does the preprocessor will throw an error
         * as either the closing '"' will not be found, or
         * if one is found the 'filename' is most likely invalid. */
        char c = advanceScanner(scanner);
        if (c == '\\' && peek(scanner) == '"')
        {
            advanceScanner(scanner);
        }
    }

    /* Expect a closing '"' to the `include` directive. */
    if (!matchScanner(scanner, '"'))
    {
        errorMsg("Expect closing '\"' for `:include` preprocessor directive.");
        goto fail;
    }

    /* No errors, valid `include` with includeStart set.
     * Check to make sure the `include` provided an actual filename
     * and not an empty string. */
    return *includeStart != scanner->current;

fail:
    hadError = true;
    return false;
}

static char*
getLitteralIncludePath(const char* start, const char* end)
{
    size_t pathLen = end - start;
    char* path = ALLOCATE(char, pathLen);
    memcpy(path, start, pathLen);
    path[pathLen] = '\0';
    return path;
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
getIncludeFilePath(const char* start, const char* end, const char* sourcePath)
{
    /* If the include path is an absolute file, just copy the path directly. */
    if (*start == '/') return getLitteralIncludePath(start, end);

    /* NOTE could implement a different function to get the sourcePath
     * length i.e. '/home/john/wks/main.wks' would return the length
     * until the last '/' in the path. In this case it would be 15.
     * Because we have to check this in either case, there may be no
     * to distinguish an absolute vs relative sourcePath. */
    size_t baseLen = getBaseDirLength(sourcePath);

    /* sourcePath is a relative file from the PWD. Just use the path
     * from the include directive. */
    if (baseLen == 0) return getLitteralIncludePath(start, end);

    /* sourcePath is not in the PWD, return the sourcePath where the include
     * should be appended to. */
    char* result = ALLOCATE(char, baseLen + (end - start) + 1); /* +1 for null byte */
    memcpy(result, sourcePath, baseLen);
    memcpy(&result[baseLen], start, end - start);
    result[baseLen + (end - start)] = '\0';
    return result;
}

char*
runPreprocessor(const char* source, const char* sourcePath, bool localDebug)
{
    assert(source);

    /* Ensure sourcePath is set. */
    if (!sourcePath) sourcePath = getenv("PWD");
    if (!sourcePath)
    {
        errorMsg("Cannot get environment variable '$PWD' required for scripts.");
        return NULL;
    }

    bool oldError = hadError;
    bool oldDebug = debug;
    debug = localDebug;
    String result = {0};
    initString(&result);

    Scanner scanner = {0};
    initScanner(&scanner, source);
    const char* scannerStart = scanner.start;
    char* includeFilePath = NULL;
    char* includeSource = NULL;
    char* includeResult = NULL;

    while (!isAtEnd(&scanner))
    {
        /* Fail on error. `hadError` is modified by `isValidInclude` */
        if (hadError) goto fail;

        const char* includeStart = NULL;
        const char* includeFileStart = NULL;

        /* If `isValidInclude` failed continue. */
        if (!isValidInclude(&scanner, &includeStart, &includeFileStart)) continue;

        /* No error, first append the current scanner contents to result. */
        appendToString(&result, scannerStart, includeStart - scannerStart);

        /* Update `scannerStart` for the next go around. */
        scannerStart = scanner.current;

        /* Get the path to the included file */
        includeFilePath = getIncludeFilePath(includeFileStart, scanner.current - 1, sourcePath);
        if (!includeFilePath)
        {
            errorMsg("Failed to get the included file path.");
            goto fail;
        }

        /* Try to read the included file */
        includeSource = readFile(includeFilePath);
        if (!includeSource) goto fail;

        /* Run preprocessor on the included file */
        includeResult = runPreprocessor(includeSource, includeFilePath, localDebug);
        if (!includeResult)
        {
            errorMsg("Failed to get preprocessor result.");
            goto fail;
        }

        /* Append the result. */
        size_t includeResultLen = strlen(includeResult);
        appendToString(&result, includeResult, includeResultLen);

        /* Cleanup allocated strings. */
        free(includeFilePath);
        free(includeSource);
        free(includeResult);
        includeFilePath = NULL;
        includeSource = NULL;
        includeResult = NULL;
    }

    /* Ensure entire source was processed. This should never execute as all
     * errors within the loop should goto fail. */
    if (!isAtEnd(&scanner))
    {
        errorMsg("Unexpected early exit while running preprocessor on file: '%s'.", sourcePath);
        goto fail;
    }

    appendToString(&result, scannerStart, scanner.current - scannerStart);

    hadError = oldError;
    debug = oldDebug;

    return result.string;

fail:
    hadError = oldError;
    debug = oldDebug;
    free(includeFilePath);
    free(includeSource);
    free(includeResult);
    freeString(&result);
    return NULL;
}
