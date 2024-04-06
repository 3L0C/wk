#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lib/common.h"

#include "preprocessor.h"
#include "lib/memory.h"
#include "scanner.h"

static bool hadError = false;

static bool
isValidInclude(Scanner* scanner, const char** includeStart)
{
    /* If not at the start of an include directive return. */
    if (!match(scanner, ':')) return false;

    /* Get the type of identifier, may be a typical hook or flag. */
    Token token = identifier(scanner);
    if (token.type != TOKEN_INCLUDE) return false;

    /* Get the first '"' after the `include` */
    while (!isAtEnd(scanner) && peek(scanner) != '"') advance(scanner);

    /* Got eof or missing beginning '"' */
    if (!match(scanner, '"'))
    {
        errorMsg("Expect '\"' after `:include` preprocessor directive.");
        hadError = true;
        return false;
    }

    /* Got starting quote, set includeStart and find closing '"' */
    *includeStart = scanner->current;
    while (!isAtEnd(scanner) && peek(scanner) != '"')
    {
        /* User's can escape double quotes in file names.
         * NOTE it is unlikely that a filename ends in a '\' character
         * but in the case where it does the preproccessor will throw an error
         * as either the closing '"' will not be found, or
         * if one is found the 'filename' is most likely invalid. */
        char c = advance(scanner);
        if (c == '\\' && peek(scanner) == '"')
        {
            advance(scanner);
        }
    }

    /* Expect a closing '"' to the `include` directive. */
    if (!match(scanner, '"'))
    {
        errorMsg("Expect closing '\"' for `:include` preprocessor directive.");
        hadError = true;
        return false;
    }

    /* No errors, valid `include` with includeStart set.
     * Check to make sure the `include` provided an actual filename
     * and not an empty string. */
    return *includeStart != scanner->current;
}

static void
appendToResult(
    const char* scannerStart,
    const char* includeStart,
    const char* includeEnd,
    char** result,
    size_t* resultCapacity,
    size_t* resultCount
)
{
    size_t scannedLen = includeStart - scannerStart;
    while (*resultCount + scannedLen > *resultCapacity)
    {
        size_t oldCapacity = *resultCapacity;
        *resultCapacity = GROW_CAPACITY(oldCapacity);
        *result = GROW_ARRAY(char, *result, oldCapacity, *resultCapacity);
    }

    memcpy(&(*result)[*resultCount], includeStart, includeEnd - includeStart);
    *resultCount += (includeEnd - includeStart);
}

static void
getPath(const char* start, const char* end, char** path, const char* sourcePath)
{
    /* If the include path is an absolute file, just copy the path directly. */
    if (*start == '/')
    {
        size_t pathLen = end - start;
        *path = ALLOCATE(char, pathLen);
        memcpy(*path, start, pathLen);
        (*path)[pathLen] = '\0';
        return;
    }
    else if (*sourcePath == '/')
    {
        /* NOTE could implement a different function to get the sourcePath
         * length i.e. '/home/john/wks/main.wks' would return the length
         * until the last '/' in the path. In this case it would be 15.
         * Because we have to check this in either case, there may be no
         * to distinguish an absolute vs relative sourcePath. */
        /* Check to see if the path is relative to */
        size_t pathLen = end - start;
        *path = ALLOCATE(char, pathLen);
        memcpy(*path, start, pathLen);
        (*path)[pathLen] = '\0';
        return;
    }
    else
    {
        /* Check to see if the path is relative to */
        size_t pathLen = end - start;
        *path = ALLOCATE(char, pathLen);
        memcpy(*path, start, pathLen);
        (*path)[pathLen] = '\0';
        return;
    }

}

char*
runPreprocessor(const char* source, const char* sourcePath)
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
    char* result = NULL;
    size_t resultCapacity = 0;
    size_t resultCount = 0;

    Scanner scanner = {0};
    initScanner(&scanner, source);
    const char* scannerStart = scanner.start;

    while (!isAtEnd(&scanner))
    {
        const char* includeStart = NULL;

        /* If `isValidInclude` failed because of an error, fail. */
        if (!isValidInclude(&scanner, &includeStart) && hadError)
        {
            goto fail;
            return NULL;
        }

        /* No error, first append the current scanner contents to result. */
        appendToResult(
            scannerStart, includeStart, scanner.current, &result, &resultCapacity, &resultCount
        );

        /* Update `scannerStart` for the next go around. */
        scannerStart = scanner.current;

        char* includePath = NULL;
        getPath(includeStart, scanner.current, &includePath, sourcePath);
    }

    hadError = oldError;
    return NULL;

fail:
    hadError = oldError;
    errorMsg("Failed but didn't cleanup! For shame...");
    return NULL;
}
