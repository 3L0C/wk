#ifndef WK_SCANNER_H_
#define WK_SCANNER_H_

#include <stdbool.h>
#include <stddef.h>

/* local includes */
#include "token.h"

typedef struct
{
    const char* head;
    const char* start;
    const char* current;
    const char* filePath;
    size_t line;
    size_t column;
    TokenType interpType;
    bool isInterpolation;
    bool hadError;
} Scanner;

void initScanner(Scanner* scanner, const char* source, const char* filePath);
bool isAtEnd(const Scanner* scanner);
void scanToken(Scanner* scanner, Token* result);

#endif /* WK_SCANNER_H_ */
