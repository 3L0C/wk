#ifndef WK_COMPILER_PIECE_TABLE_H_
#define WK_COMPILER_PIECE_TABLE_H_

#include <stddef.h>

/* common includes */
#include "common/string.h"

typedef enum
{
    PIECE_SOURCE_ORIGINAL,
    PIECE_SOURCE_ADD,
} PieceSource;

typedef struct
{
    PieceSource source;
    size_t index;
    size_t len;
} Piece;

typedef struct
{
    Piece* pieces;
    size_t capacity;
    size_t count;
} PieceArray;

typedef struct
{
    const char* original;
    size_t originalLen;
    String add;
    PieceArray pieces;
} PieceTable;

void appendToPieceTable(PieceTable* pieceTable, PieceSource source, const char* text, size_t len);
char* compilePieceTableToString(PieceTable* pieceTable);
void freePieceTable(PieceTable* pieceTable);
const char* getTextAtPiece(PieceTable* pieceTable, Piece* piece);
void initPieceTable(PieceTable* pieceTable, const char* original);

#endif /* WK_COMPILER_PIECE_TABLE_H_ */
