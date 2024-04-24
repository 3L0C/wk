#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

/* common includes */
#include "common/debug.h"

/* local includes */
#include "debug.h"
#include "piece_table.h"
#include "token.h"

void
debugPrintScannedTokenFooter(void)
{
    debugMsg(true, "|           |                             |                            |");
    debugPrintHeader("");
}

void
debugPrintScannedTokenHeader(void)
{
    debugPrintHeader("-");
    debugMsg(true, "|                            Scanned Tokens                            |");
    debugPrintHeader("-");
    debugMsg(true, "| Line:Col  |          TokenType          |           Lexeme           |");
    debugPrintHeader("-");
    debugMsg(true, "|           |                             |                            |");
}

void
disassemblePiece(const PieceTable* pieceTable, size_t index)
{
    assert(pieceTable);

    Piece* piece = &pieceTable->pieces.pieces[index];
    debugMsg(true, "| ");
    debugMsg(
        true,
        "| Source:   %s",
        piece->source == PIECE_SOURCE_ORIGINAL
        ? "ORIGINAL" : "ADD"
    );
    debugMsg(true, "| Index:    %04zu", piece->index);
    debugMsg(true, "| Length:   %04zu", piece->len);
    if (piece->len > 0)
    {
        debugMsg(true, "| Text: ");
        debugMsg(true, "| ");
        debugTextLenWithLineNumber(getTextAtPiece(pieceTable, piece), piece->len);
    }
}

static void
disassemblePieceArray(const PieceTable* pieceTable)
{
    assert(pieceTable);

    const PieceArray* array = &pieceTable->pieces;
    for (size_t i = 0; i < array->count; i++)
    {
        debugMsg(true, "|----------- Piece number: %04zu ------------", i);
        disassemblePiece(pieceTable, i);
        debugMsg(true, "| ");
    }
}

void
disassemblePieceTable(const PieceTable* pieceTable)
{
    assert(pieceTable);

    debugPrintHeader(" PieceTable ");
    debugMsg(true, "| ");
    debugMsg(true, "| Total pieces:         %zu", pieceTable->pieces.count);
    debugMsg(true, "| Original Text length: %zu", pieceTable->originalLen);
    debugMsg(true, "| Add Text length:      %zu", pieceTable->add.count);
    debugMsg(true, "| ");
    debugMsg(true, "|-------------- Original Text --------------");
    debugMsg(true, "| ");
    debugTextWithLineNumber(pieceTable->original);
    debugMsg(true, "| ");
    if (pieceTable->add.string)
    {
        debugMsg(true, "|---------------- Add Text -----------------");
        debugMsg(true, "| ");
        debugTextWithLineNumber(pieceTable->add.string);
        debugMsg(true, "| ");
    }
    disassemblePieceArray(pieceTable);
    debugPrintHeader("");
}

void
disassemblePseudoChord(const PseudoChord* chord)
{
    assert(chord);

    debugPrintHeader(" PseudoChord ");
    debugMsg(true, "|");
    debugMsg(
        true, "| State:             %s",
        chord->state == KEY_CHORD_STATE_IS_NULL ? "STATE_IS_NULL" : "STATE_NOT_NULL"
    );
    disassembleKeyWithoutHeader(&chord->key, 0);
    disassembleTokenArray(&chord->description);
    disassembleTokenArray(&chord->command);
    disassembleTokenArray(&chord->before);
    disassembleTokenArray(&chord->after);
    disassembleFlags(&chord->flags, 0);
    debugMsg(true, "|");
    debugPrintHeader("");
}

void
disassembleScanner(const Scanner* scanner)
{
    assert(scanner);

    debugPrintHeader(" Scanner ");
    debugMsg(true, "|");
    debugMsg(true, "| Head:              '%c'", *scanner->head);
    debugMsg(true, "| Start:             '%c'", *scanner->start);
    debugMsg(true, "| Current:           '%c'", *scanner->current);
    debugMsg(true, "| Lexeme:            '%.*s'", scanner->current - scanner->start, scanner->start);
    debugMsg(true, "| Filepath:          '%s'", scanner->filepath);
    debugMsg(true, "| Line:              %zu", scanner->line);
    debugMsg(true, "| Column:            %zu", scanner->column);
    debugMsg(true, "| Had Error:         %s", scanner->hadError ? "true" : "false");
    /* debugMsg(true, "| State:             %s", *scanner->head); */
    /* debugMsg(true, "| Prev State:        %s", *scanner->head); */
    debugMsg(true, "| Interp Type:       %s", getTokenLiteral(scanner->interpType));
    debugMsg(true, "|");
    debugPrintHeader("");
}

void
disassembleSingleToken(const Token* token)
{
    assert(token);

    debugPrintHeader("-");
    disassembleToken(token);
    debugPrintHeader("");
}

static void
printErrorToken(const Token* token)
{
    assert(token);

    debugMsg(
        true,
        "| %04zu:%04zu | %-27s | %-26.*s |",
        token->line, token->column, "TOKEN_ERROR",
        (int)token->length, token->start
    );
}

static void
printSimpleToken(const Token* token)
{
    assert(token);

    debugMsg(
        true,
        "| %04zu:%04zu | %-27s | %-26.*s |",
        token->line, token->column, getTokenLiteral(token->type),
        (int)token->length, token->start
    );
}

void
disassembleToken(const Token* token)
{
    assert(token);

    if (token->type == TOKEN_ERROR) printErrorToken(token);
    else printSimpleToken(token);
}

void
disassembleTokenArray(const TokenArray* tokens)
{
    assert(tokens);

    if (tokens->count == 0) return;

    for (size_t i = 0; i < tokens->count; i++)
    {
        disassembleToken(&tokens->tokens[i]);
    }
}
