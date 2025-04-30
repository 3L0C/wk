#ifndef WK_COMPILER_DEBUG_H_
#define WK_COMPILER_DEBUG_H_

#include "scanner.h"
#include "token.h"

void debugPrintScannedTokenHeader(void);
void debugPrintScannedTokenFooter(void);
void disassembleScanner(const Scanner* scanner);
void disassembleSingleToken(const Token* token);
void disassembleToken(const Token* token);

#endif /* WK_COMPILER_DEBUG_H_ */
