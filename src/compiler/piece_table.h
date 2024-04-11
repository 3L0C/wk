#ifndef WK_COMPILER_PIECE_TABLE_H_
#define WK_COMPILER_PIECE_TABLE_H_

#include "common/string.h"
#include <stddef.h>

typedef enum
{
    PIECE_SOURCE_ORIGINAL,
    PIECE_SOURCE_ADD,
} PieceSource;

typedef struct
{
    PieceSource source;
    const char* start;
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
void initPieceTable(PieceTable* pieceTable, const char* original);

#endif /* WK_COMPILER_PIECE_TABLE_H_ */
