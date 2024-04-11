#include <assert.h>
#include <string.h>

/* common includes */
#include "common/string.h"
#include "common/memory.h"

/* local includes */
#include "piece_table.h"

static bool
isWithinOriginalBounds(PieceTable* pieceTable, const char* text)
{
    const char* original = pieceTable->original;
    size_t originalLen = pieceTable->originalLen;
    return (!(text < original) && !(text > &original[originalLen]));
}

static void
copyPiece(Piece* from, Piece* to)
{
    assert(from && to);

    to->source = from->source;
    to->start = from->start;
    to->len = from->len;
}

static void
writePieceToPieceArray(PieceArray* array, PieceSource source, const char* text, size_t len)
{
    if (array->count == array->capacity)
    {
        size_t oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->pieces = GROW_ARRAY(Piece, array->pieces, oldCapacity, array->capacity);
    }

    Piece piece = { source, text, len };
    copyPiece(&piece, &array->pieces[array->count]);
    array->count++;
}

void
appendToPieceTable(PieceTable* pieceTable, PieceSource source, const char* text, size_t len)
{
    assert(pieceTable);

    if (source == PIECE_SOURCE_ORIGINAL)
    {
        assert(isWithinOriginalBounds(pieceTable, text));
        writePieceToPieceArray(&pieceTable->pieces, source, text, len);
        return;
    }

    String* add = &pieceTable->add;
    appendToString(add, text, len);
    writePieceToPieceArray(&pieceTable->pieces, source, &add->string[add->count - len], len);
}

static void
initPieceArray(PieceArray* array)
{
    assert(array);

    array->pieces = NULL;
    array->count = 0;
    array->capacity = 0;
}

void
initPieceTable(PieceTable* pieceTable, const char* original)
{
    assert(pieceTable && original);

    pieceTable->original = original;
    pieceTable->originalLen = strlen(original);
    initString(&pieceTable->add);
    initPieceArray(&pieceTable->pieces);
}
