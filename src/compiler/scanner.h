#ifndef WK_COMPILER_SCANNER_H_
#define WK_COMPILER_SCANNER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* local includes */
#include "token.h"

typedef uint8_t ScannerFlag;
enum
{
    SCANNER_WANTS_MACRO,
    SCANNER_WANTS_DESCRIPTION,
    SCANNER_WANTS_DOUBLE,
    SCANNER_WANTS_INTEGER,
    SCANNER_WANTS_UNSIGNED_INTEGER,
};

typedef uint8_t ScannerState;
enum
{
    SCANNER_STATE_NORMAL,
    SCANNER_STATE_DESCRIPTION,
    SCANNER_STATE_COMMAND,
    SCANNER_STATE_INTERPOLATION,
};

typedef struct
{
    const char* head;
    const char* start;
    const char* current;
    const char* filepath;
    size_t line;
    size_t column;
    ScannerState state;
    ScannerState previousState;
    TokenType interpType;
    bool hadError;
} Scanner;

void scannerInit(Scanner* scanner, const char* source, const char* filepath);
void scannerClone(const Scanner* scanner, Scanner* clone);
bool scannerIsAtEnd(const Scanner* scanner);
void scannerMakeCurrent(Scanner* scanner);
void scannerGetTokenForCompiler(Scanner* scanner, Token* result);
void scannerGetTokenForPreprocessor(Scanner* scanner, Token* result, ScannerFlag flag);

#endif /* WK_COMPILER_SCANNER_H_ */
