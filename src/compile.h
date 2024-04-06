#ifndef WK_COMPILE_H_
#define WK_COMPILE_H_

#include "lib/menu.h"

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

#endif /* WK_COMPILE_H_ */
