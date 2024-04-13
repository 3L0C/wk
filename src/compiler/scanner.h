#ifndef WK_COMPILER_SCANNER_H_
#define WK_COMPILER_SCANNER_H_

#include <stdbool.h>
#include <stddef.h>

/* local includes */
#include "token.h"

typedef struct
{
    const char* head;
    const char* start;
    const char* current;
    const char* filepath;
    size_t line;
    size_t column;
    TokenType interpType;
    bool isInterpolation;
    bool hadError;
} Scanner;

void initScanner(Scanner* scanner, const char* source, const char* filepath);
bool isAtEnd(const Scanner* scanner);
void scanTokenForCompiler(Scanner* scanner, Token* result);
void scanTokenForPreprocessor(Scanner* scanner, Token* result, bool wantsDescription);

#endif /* WK_COMPILER_SCANNER_H_ */
