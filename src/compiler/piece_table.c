#include <assert.h>
#include <stdbool.h>
#include <string.h>

/* common includes */
#include "common/string.h"
#include "common/memory.h"

/* local includes */
#include "piece_table.h"

static bool
isWithinOriginalBounds(PieceTable* pieceTable, const char* text)
{
    assert(pieceTable);

    return (
        !(text < pieceTable->original) &&
        !(text > pieceTable->original + pieceTable->originalLen)
    );
}

static void
copyPiece(Piece* from, Piece* to)
{
    assert(from), assert(to);

    to->source = from->source;
    to->index = from->index;
    to->len = from->len;
}

static void
writePieceToPieceArray(PieceArray* array, PieceSource source, size_t index, size_t len)
{
    assert(array);

    if (array->count == array->capacity)
    {
        size_t oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->pieces = GROW_ARRAY(Piece, array->pieces, oldCapacity, array->capacity);
    }

    Piece piece = { source, index, len };
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
        size_t index = text - pieceTable->original;
        writePieceToPieceArray(&pieceTable->pieces, source, index, len);
        return;
    }

    String* add = &pieceTable->add;
    size_t start = add->count;
    appendToString(add, text, len);
    writePieceToPieceArray(&pieceTable->pieces, source, start, len);
}

char*
compilePieceTableToString(const PieceTable* pieceTable)
{
    assert(pieceTable);

    String result;
    initString(&result);
    const PieceArray* pieces = &pieceTable->pieces;
    for (size_t i = 0; i < pieces->count; i++)
    {
        Piece* piece = &pieces->pieces[i];
        appendToString(&result, getTextAtPiece(pieceTable, piece), piece->len);
    }

    return result.string;
}

static void
freePieceArray(PieceArray* array)
{
    assert(array);

    FREE_ARRAY(PieceArray, array->pieces, array->capacity);
    array->pieces = NULL;
    array->count = 0;
    array->capacity = 0;
}

void
freePieceTable(PieceTable* pieceTable)
{
    assert(pieceTable);

    freeString(&pieceTable->add);
    freePieceArray(&pieceTable->pieces);
}

const char*
getTextAtPiece(const PieceTable* pieceTable, Piece* piece)
{
    assert(pieceTable), assert(piece);

    return (
        piece->source == PIECE_SOURCE_ADD
        ? pieceTable->add.string + piece->index
        : pieceTable->original + piece->index
    );
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
    assert(pieceTable), assert(original);

    pieceTable->original = original;
    pieceTable->originalLen = strlen(original);
    initString(&pieceTable->add);
    initPieceArray(&pieceTable->pieces);
}
