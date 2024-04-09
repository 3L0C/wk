#ifndef WK_PARSER_H_
#define WK_PARSER_H_

#include "scanner.h"

typedef struct
{
    Scanner* scanner;
    Token current;
    Token previous;
    bool  hadError;
    bool  panicMode;
} Parser;

#endif /* WK_PARSER_H_ */
