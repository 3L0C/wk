#ifndef WK_COMPILER_DEBUG_H_
#define WK_COMPILER_DEBUG_H_

#include "line.h"

void disassembleToken(Token* token);
void disassembleLine(Line* line, size_t index);
void disassembleLineShallow(Line* line, size_t index);
void debugLineArray(LineArray* array);

#endif /* WK_COMPILER_DEBUG_H_ */
