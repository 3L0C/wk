#include <assert.h>
#include <stddef.h>
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
    char* result = ALLOCATE(char, baseLen + (end - start));
    memcpy(result, sourcePath, baseLen);
    memcpy(&result[baseLen], start, end - start);
    result[baseLen + (end - start)] = '\0';
    return result;
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

        /* Get the path to the included file */
        char* includePath = getIncludeFilePath(includeStart, scanner.current, sourcePath);

    }

    hadError = oldError;
    return NULL;

fail:
    hadError = oldError;
    errorMsg("Failed but didn't cleanup! For shame...");
    return NULL;
}
