#ifndef WK_DEBUG_H_
#define WK_DEBUG_H_

#include "line.h"
#include "scanner.h"

void disassembleToken(Token* token);
void debugScanner(const char* source);
void disassembleLine(Line* line, size_t index);
void debugLineArray(LineArray* array);

#endif /* WK_DEBUG_H_ */
