#ifndef WK_COMPILER_COMPILER_H_
#define WK_COMPILER_COMPILER_H_

/* common includes */
#include "common/arena.h"
#include "common/array.h"
#include "common/menu.h"

/* local includes */
#include "token.h"
#include "scanner.h"

typedef struct
{
    Scanner sourceScanner;
    Scanner implicitArrayKeysScanner;
    Token current;
    Token previous;
    Array implicitKeys;
    Array* dest;
    Array* chords;
    Arena* arena;
    Scanner* scanner;
    const char* delimiter;
    char* source;
    size_t delimiterLen;
    bool hadError;
    bool panicMode;
    bool sort;
    bool debug;
} Compiler;

Array* compileKeyChords(Compiler* compiler, Menu* menu);
void initCompiler(Compiler* compiler, Menu* menu, char* source, const char* filepath);


#endif /* WK_COMPILER_COMPILER_H_ */
