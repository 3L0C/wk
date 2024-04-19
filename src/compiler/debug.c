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
printSimpleToken(const Token* token, const char* type)
{
    assert(token);

    debugMsg(
        true,
        "| %04zu:%04zu | %-27s | %-26.*s |",
        token->line, token->column, type,
        (int)token->length, token->start
    );
}

void
disassembleToken(const Token* token)
{
    assert(token);

    char* type = "TOKEN_UNKNOWN";
    switch (token->type)
    {
    /* single characters */
    case TOKEN_LEFT_BRACKET: type = "TOKEN_LEFT_BRACKET"; break;
    case TOKEN_RIGHT_BRACKET: type = "TOKEN_RIGHT_BRACKET"; break;
    case TOKEN_LEFT_BRACE: type = "TOKEN_LEFT_BRACE"; break;
    case TOKEN_RIGHT_BRACE: type = "TOKEN_RIGHT_BRACE"; break;
    case TOKEN_LEFT_PAREN: type = "TOKEN_LEFT_PAREN"; break;
    case TOKEN_RIGHT_PAREN: type = "TOKEN_RIGHT_PAREN"; break;

    /* preprocessor macros */

    /* string macros */
    case TOKEN_INCLUDE: type = "TOKEN_INCLUDE"; break;
    case TOKEN_FOREGROUND_COLOR: type = "TOKEN_FOREGROUND_COLOR"; break;
    case TOKEN_BACKGROUND_COLOR: type = "TOKEN_BACKGROUND_COLOR"; break;
    case TOKEN_BORDER_COLOR: type = "TOKEN_BORDER_COLOR"; break;
    case TOKEN_SHELL: type = "TOKEN_SHELL"; break;
    case TOKEN_FONT: type = "TOKEN_FONT"; break;

    /* switch macros */
    case TOKEN_DEBUG: type = "TOKEN_DEBUG"; break;
    case TOKEN_SORT: type = "TOKEN_SORT"; break;
    case TOKEN_TOP: type = "TOKEN_TOP"; break;
    case TOKEN_BOTTOM: type = "TOKEN_BOTTOM"; break;

    /* [-]digit macros */
    case TOKEN_WINDOW_WIDTH: type = "TOKEN_WINDOW_WIDTH"; break;
    case TOKEN_WINDOW_GAP: type = "TOKEN_WINDOW_GAP"; break;

    /* digit macros */
    case TOKEN_MAX_COLUMNS: type = "TOKEN_MAX_COLUMNS"; break;
    case TOKEN_BORDER_WIDTH: type = "TOKEN_BORDER_WIDTH"; break;
    case TOKEN_WIDTH_PADDING: type = "TOKEN_WIDTH_PADDING"; break;
    case TOKEN_HEIGHT_PADDING: type = "TOKEN_HEIGHT_PADDING"; break;

    /* number macros */
    case TOKEN_BORDER_RADIUS: type = "TOKEN_BORDER_RADIUS"; break;

    /* identifiers */
    case TOKEN_THIS_KEY: type = "TOKEN_THIS_KEY"; break;
    case TOKEN_INDEX: type = "TOKEN_INDEX"; break;
    case TOKEN_INDEX_ONE: type = "TOKEN_INDEX_ONE"; break;
    case TOKEN_THIS_DESC: type = "TOKEN_THIS_DESC"; break;
    case TOKEN_THIS_DESC_UPPER_FIRST: type = "TOKEN_THIS_DESC_UPPER_FIRST"; break;
    case TOKEN_THIS_DESC_LOWER_FIRST: type = "TOKEN_THIS_DESC_LOWER_FIRST"; break;
    case TOKEN_THIS_DESC_UPPER_ALL: type = "TOKEN_THIS_DESC_UPPER_ALL"; break;
    case TOKEN_THIS_DESC_LOWER_ALL: type = "TOKEN_THIS_DESC_LOWER_ALL"; break;

    /* hooks */
    case TOKEN_BEFORE: type = "TOKEN_BEGIN"; break;
    case TOKEN_AFTER: type = "TOKEN_BEGIN"; break;
    case TOKEN_SYNC_BEFORE: type = "TOKEN_SYNC_BEFORE"; break;
    case TOKEN_SYNC_AFTER: type = "TOKEN_SYNC_AFTER"; break;

    /* flags */
    case TOKEN_KEEP: type = "TOKEN_KEEP"; break;
    case TOKEN_CLOSE: type = "TOKEN_CLOSE"; break;
    case TOKEN_INHERIT: type = "TOKEN_INHERIT"; break;
    case TOKEN_IGNORE: type = "TOKEN_IGNORE"; break;
    case TOKEN_UNHOOK: type = "TOKEN_UNHOOK"; break;
    case TOKEN_DEFLAG: type = "TOKEN_DEFLAG"; break;
    case TOKEN_NO_BEFORE: type = "TOKEN_NO_BEFORE"; break;
    case TOKEN_NO_AFTER: type = "TOKEN_NO_AFTER"; break;
    case TOKEN_WRITE: type = "TOKEN_WRITE"; break;
    case TOKEN_SYNC_CMD: type = "TOKEN_SYNC_CMD"; break;

    /* literals */
    case TOKEN_COMMAND: type = "TOKEN_COMMAND"; break;
    case TOKEN_DESCRIPTION: type = "TOKEN_DESCRIPTION"; break;
    case TOKEN_DOUBLE: type = "TOKEN_DOUBLE"; break;
    case TOKEN_INTEGER: type = "TOKEN_INTEGER"; break;
    case TOKEN_UNSIGNED_INTEGER: type = "TOKEN_UNSIGNED_INTEGER"; break;

    /* interpolations */
    case TOKEN_COMM_INTERP: type = "TOKEN_COMM_INTERP"; break;
    case TOKEN_DESC_INTERP: type = "TOKEN_DESC_INTERP"; break;

    /* keys */
    case TOKEN_KEY: type = "TOKEN_KEY"; break;

    /* mods */
    case TOKEN_MOD_CTRL: type = "TOKEN_MOD_CTRL"; break;
    case TOKEN_MOD_ALT: type = "TOKEN_MOD_ALT"; break;
    case TOKEN_MOD_HYPER: type = "TOKEN_MOD_HYPER"; break;
    case TOKEN_MOD_SHIFT: type = "TOKEN_MOD_SHIFT"; break;

    /* specials */
    case TOKEN_SPECIAL_KEY: type = "TOKEN_SPECIAL_KEY"; break;

    /* control */
    case TOKEN_NO_INTERP: type = "TOKEN_NO_INTERP";   break;
    case TOKEN_EMPTY: type = "TOKEN_EMPTY"; break;
    case TOKEN_ERROR: printErrorToken(token); return;
    case TOKEN_EOF: type = "TOKEN_EOF"; break;
    }
    printSimpleToken(token, type);
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
