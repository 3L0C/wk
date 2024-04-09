#ifndef WK_COMPILER_COMPILE_H_
#define WK_COMPILER_COMPILE_H_

/* common includes */
#include "common/menu.h"

/* local includes */
#include "line.h"
#include "scanner.h"

typedef struct
{
    Scanner     scanner;
    Token       current;
    Token       previous;
    bool        hadError;
    bool        panicMode;
    int         index;
    Line        line;
    LineArray*  lineDest;
    LineArray*  linePrefix;
    LineArray   lines;
} Compiler;

bool compileChords(Compiler* compiler, WkMenu* menu);
void initCompiler(Compiler* compiler, const char* source);

#endif /* WK_COMPILER_COMPILE_H_ */
