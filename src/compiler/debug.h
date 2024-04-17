#ifndef WK_COMPILER_DEBUG_H_
#define WK_COMPILER_DEBUG_H_

#include "compiler.h"
#include "piece_table.h"
#include "token.h"

void debugPrintScannedTokenHeader(void);
void debugPrintScannedTokenFooter(void);
void disassemblePiece(const PieceTable* pieceTable, size_t index);
void disassemblePieceTable(const PieceTable* pieceTable);
void disassemblePseudoChord(const PseudoChord* chord);
void disassembleToken(const Token* token);
void disassembleSingleToken(const Token* token);
void disassembleTokenArray(const TokenArray* tokens);

#endif /* WK_COMPILER_DEBUG_H_ */
