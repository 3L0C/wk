#ifndef WK_COMPILER_DEBUG_H_
#define WK_COMPILER_DEBUG_H_

#include "line.h"
#include "piece_table.h"
#include "token.h"

void debugPrintScannedTokenHeader(void);
void debugPrintScannedTokenFooter(void);
void disassembleLine(Line* line, size_t index, int indent);
void disassembleLineArray(LineArray* array, int indent);
void disassembleLineShallow(Line* line, size_t index);
void disassemblePiece(PieceTable* pieceTable, size_t index);
void disassemblePieceTable(PieceTable* pieceTable);
void disassembleToken(Token* token);

#endif /* WK_COMPILER_DEBUG_H_ */
