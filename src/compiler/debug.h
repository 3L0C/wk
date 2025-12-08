#ifndef WK_COMPILER_DEBUG_H_
#define WK_COMPILER_DEBUG_H_

#include "lazy_string.h"
#include "scanner.h"
#include "token.h"

void debugPrintScannedTokenFooter(void);
void debugPrintScannedTokenHeader(void);
void disassembleLazyString(const LazyString* string, const char* title, int indent);
void disassembleScanner(const Scanner* scanner);
void disassembleSingleToken(const Token* token);
void disassembleToken(const Token* token);

#endif /* WK_COMPILER_DEBUG_H_ */
