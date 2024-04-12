#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* common includes */
#include "common/common.h"
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
handleIncludeMacro(WkMenu* menu, Scanner* scanner, const char* scannerStart, PieceTable* result)
{
    assert(menu && scanner && scannerStart && result);

    /* currently pointing at the 'i' in ':include', so take off one. */
    const char* includeStart = scanner->start - 1;
    const char* sourcePath = scanner->filePath;

    /* Ensure filename is given. */
    Token includeFile = {0};
    initToken(&includeFile);
    scanToken(scanner, &includeFile);
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

    /* Append previous contents to result. */
    appendToPieceTable(result, PIECE_SOURCE_ORIGINAL, scannerStart, includeStart - scannerStart);

    char* includeFilePath = NULL;
    char* includeSource = NULL; // readFile(includeFilePath);
    char* includeResult = NULL;

    /* Get the path to the included file */
    includeFilePath = getIncludeFilePath(includeFile.start, includeFile.length, sourcePath);
    if (!includeFilePath)
    {
        errorMsg("Failed to get the included file path.");
        goto fail;
    }

    /* Try to read the included file */
    includeSource = readFile(includeFilePath);
    if (!includeSource) goto fail; /* readFile prints an error for us. */

    /* check that the file and the source are not the one and the same */
    if (strcmp(includeSource, scanner->head) == 0)
    {
        errorMsg(
            "Included file appears to be the same as the source file. Cannot `:include` self."
        );
        goto fail;
    }

    /* Run preprocessor on the included file */
    includeResult = runPreprocessor(menu, includeSource, includeFilePath);
    if (!includeResult)
    {
        errorMsg("Failed to get preprocessor result.");
        goto fail;
    }

    /* Append the result. */
    appendToPieceTable(result, PIECE_SOURCE_ADD, includeResult, strlen(includeResult));

    free(includeFilePath);
    free(includeSource);
    free(includeResult);
    return;

fail:
    free(includeFilePath);
    free(includeSource);
    free(includeResult);
    scanner->hadError = true;
}

char*
runPreprocessor(WkMenu* menu, const char* source, const char* sourcePath)
{
    assert(menu && source);

    Scanner scanner = {0};
    initScanner(&scanner, source, sourcePath);
    PieceTable pieceTable;
    initPieceTable(&pieceTable, source);
    const char* scannerStart = scanner.head;

    while (!isAtEnd(&scanner))
    {
        if (scanner.hadError) goto fail;

        Token token = {0};
        scanToken(&scanner, &token);
        switch (token.type)
        {
        case TOKEN_INCLUDE:
        {
            handleIncludeMacro(menu, &scanner, scannerStart, &pieceTable);
            /* Update scanner for the next go around. */
            scannerStart = scanner.current;
            break;
        }
        default:
        {
            if (menu->debug)
            {
                /* DEBUG HERE */
            }
            break;
        }
        }
    }

    /* Append the last bit of the source to result. */
    appendToPieceTable(
        &pieceTable, PIECE_SOURCE_ORIGINAL, scannerStart, scanner.current - scannerStart
    );

    if (menu->debug) disassemblePieceTable(&pieceTable);
    char* result = compilePieceTableToString(&pieceTable);
    freePieceTable(&pieceTable);
    return result;

fail:
    freePieceTable(&pieceTable);
    return NULL;
}
