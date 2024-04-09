#ifndef WK_TOKEN_H_
#define WK_TOKEN_H_

#include <stddef.h>

#include "scanner.h"

typedef struct
{
    Token* tokens;
    size_t capacity;
    size_t count;
} TokenArray;

void copyTokenArray(TokenArray* from, TokenArray* to);
void initTokenArray(TokenArray* array);
void freeTokenArray(TokenArray* array);
void writeTokenArray(TokenArray* array, Token* token);

#endif /* WK_TOKEN_H_ */
