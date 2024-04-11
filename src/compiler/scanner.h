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
    size_t line;
    TokenType interpType;
    bool isInterpolation;
    bool hadError;
} Scanner;

void initScanner(Scanner* scanner, const char* source);
bool isAtEnd(const Scanner* scanner);
void scanToken(Scanner* scanner, Token* result);

#endif /* WK_SCANNER_H_ */
