#ifndef WK_COMPILER_COMPILE_H_
#define WK_COMPILER_COMPILE_H_

#include <stdint.h>

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
    uint32_t    index;
    Line        line;
    LineArray*  lineDest;
    LineArray*  linePrefix;
    LineArray   lines;
    char*       source;
} Compiler;

bool compileKeyChords(Compiler* compiler, WkMenu* menu);
void initCompiler(Compiler* compiler, char* source, const char* filepath);

#endif /* WK_COMPILER_COMPILE_H_ */
