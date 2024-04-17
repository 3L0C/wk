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
    debugMsg(true, "| ");
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
    debugMsg(true, "| ");
    debugPrintHeader("");
}

static void
printErrorToken(const Token* token)
{
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

    char* type = "";
    switch (token->type)
    {
    /* single characters */
    case TOKEN_LEFT_BRACKET:            type = "TOKEN_LEFT_BRACKET";            break;
    case TOKEN_RIGHT_BRACKET:           type = "TOKEN_RIGHT_BRACKET";           break;
    case TOKEN_LEFT_BRACE:              type = "TOKEN_LEFT_BRACE";              break;
    case TOKEN_RIGHT_BRACE:             type = "TOKEN_RIGHT_BRACE";             break;
    case TOKEN_LEFT_PAREN:              type = "TOKEN_LEFT_PAREN";              break;
    case TOKEN_RIGHT_PAREN:             type = "TOKEN_RIGHT_PAREN";             break;

    /* preprocessor macros */
    case TOKEN_INCLUDE:                 type = "TOKEN_INCLUDE";                 break;
    case TOKEN_DEBUG:                   type = "TOKEN_DEBUG";                   break;
    case TOKEN_TOP:                     type = "TOKEN_TOP";                     break;
    case TOKEN_BOTTOM:                  type = "TOKEN_BOTTOM";                  break;
    case TOKEN_MAX_COLUMNS:             type = "TOKEN_MAX_COLUMNS";             break;
    case TOKEN_WINDOW_WIDTH:            type = "TOKEN_WINDOW_WIDTH";            break;
    case TOKEN_WINDOW_GAP:              type = "TOKEN_WINDOW_GAP";              break;
    case TOKEN_BORDER_WIDTH:            type = "TOKEN_BORDER_WIDTH";            break;
    case TOKEN_BORDER_RADIUS:           type = "TOKEN_BORDER_RADIUS";           break;
    case TOKEN_WIDTH_PADDING:           type = "TOKEN_WIDTH_PADDING";           break;
    case TOKEN_HEIGHT_PADDING:          type = "TOKEN_HEIGHT_PADDING";          break;
    case TOKEN_FOREGROUND_COLOR:        type = "TOKEN_FOREGROUND_COLOR";        break;
    case TOKEN_BACKGROUND_COLOR:        type = "TOKEN_BACKGROUND_COLOR";        break;
    case TOKEN_BORDER_COLOR:            type = "TOKEN_BORDER_COLOR";            break;
    case TOKEN_SHELL:                   type = "TOKEN_SHELL";                   break;
    case TOKEN_FONT:                    type = "TOKEN_FONT";                    break;

    /* identifiers */
    case TOKEN_THIS_KEY:                type = "TOKEN_THIS_KEY";                break;
    case TOKEN_INDEX:                   type = "TOKEN_INDEX";                   break;
    case TOKEN_INDEX_ONE:               type = "TOKEN_INDEX_ONE";               break;
    case TOKEN_THIS_DESC:               type = "TOKEN_THIS_DESC";               break;
    case TOKEN_THIS_DESC_UPPER_FIRST:   type = "TOKEN_THIS_DESC_UPPER_FIRST";   break;
    case TOKEN_THIS_DESC_LOWER_FIRST:   type = "TOKEN_THIS_DESC_LOWER_FIRST";   break;
    case TOKEN_THIS_DESC_UPPER_ALL:     type = "TOKEN_THIS_DESC_UPPER_ALL";     break;
    case TOKEN_THIS_DESC_LOWER_ALL:     type = "TOKEN_THIS_DESC_LOWER_ALL";     break;

    /* keywords */
    case TOKEN_BEFORE:                  type = "TOKEN_BEGIN";                   break;
    case TOKEN_AFTER:                   type = "TOKEN_BEGIN";                   break;
    case TOKEN_KEEP:                    type = "TOKEN_KEEP";                    break;
    case TOKEN_INHERIT:                 type = "TOKEN_INHERIT";                 break;
    case TOKEN_IGNORE:                  type = "TOKEN_IGNORE";                  break;
    case TOKEN_UNHOOK:                  type = "TOKEN_UNHOOK";                  break;
    case TOKEN_DEFLAG:                  type = "TOKEN_DEFLAG";                  break;
    case TOKEN_NO_BEFORE:               type = "TOKEN_NO_BEFORE";               break;
    case TOKEN_NO_AFTER:                type = "TOKEN_NO_AFTER";                break;
    case TOKEN_WRITE:                   type = "TOKEN_WRITE";                   break;
    case TOKEN_SYNC_CMD:                type = "TOKEN_SYNC_CMD";                break;
    case TOKEN_SYNC_BEFORE:             type = "TOKEN_SYNC_BEFORE";             break;
    case TOKEN_SYNC_AFTER:              type = "TOKEN_SYNC_AFTER";              break;

    /* literals */
    case TOKEN_COMMAND:                 type = "TOKEN_COMMAND";                 break;
    case TOKEN_COMM_INTERP:             type = "TOKEN_COMM_INTERP";             break;
    case TOKEN_DESCRIPTION:             type = "TOKEN_DESCRIPTION";             break;
    case TOKEN_DESC_INTERP:             type = "TOKEN_DESC_INTERP";             break;

    /* keys */
    case TOKEN_KEY:                     type = "TOKEN_KEY";                     break;

    /* mods */
    case TOKEN_MOD_CTRL:                type = "TOKEN_MOD_CTRL";                break;
    case TOKEN_MOD_ALT:                 type = "TOKEN_MOD_ALT";                 break;
    case TOKEN_MOD_HYPER:               type = "TOKEN_MOD_HYPER";               break;
    case TOKEN_MOD_SHIFT:               type = "TOKEN_MOD_SHIFT";               break;

    /* specials */
    case TOKEN_SPECIAL_LEFT:            type = "TOKEN_SPECIAL_LEFT";            break;
    case TOKEN_SPECIAL_RIGHT:           type = "TOKEN_SPECIAL_RIGHT";           break;
    case TOKEN_SPECIAL_UP:              type = "TOKEN_SPECIAL_UP";              break;
    case TOKEN_SPECIAL_DOWN:            type = "TOKEN_SPECIAL_DOWN";            break;
    case TOKEN_SPECIAL_TAB:             type = "TOKEN_SPECIAL_TAB";             break;
    case TOKEN_SPECIAL_SPACE:           type = "TOKEN_SPECIAL_SPACE";           break;
    case TOKEN_SPECIAL_RETURN:          type = "TOKEN_SPECIAL_RETURN";          break;
    case TOKEN_SPECIAL_DELETE:          type = "TOKEN_SPECIAL_DELETE";          break;
    case TOKEN_SPECIAL_ESCAPE:          type = "TOKEN_SPECIAL_ESCAPE";          break;
    case TOKEN_SPECIAL_HOME:            type = "TOKEN_SPECIAL_HOME";            break;
    case TOKEN_SPECIAL_PAGE_UP:         type = "TOKEN_SPECIAL_PAGE_UP";         break;
    case TOKEN_SPECIAL_PAGE_DOWN:       type = "TOKEN_SPECIAL_PAGE_DOWN";       break;
    case TOKEN_SPECIAL_END:             type = "TOKEN_SPECIAL_END";             break;
    case TOKEN_SPECIAL_BEGIN:           type = "TOKEN_SPECIAL_BEGIN";           break;

    /* control */
    case TOKEN_EOF:                                     return;
    case TOKEN_ERROR:       printErrorToken(token);     return;
    case TOKEN_NO_INTERP:   type = "TOKEN_NO_INTERP";   break;

    default: type = "TOKEN_UNKNOWN!"; break;
    }
    printSimpleToken(token, type);
}


void
disassembleSingleToken(const Token* token)
{
    assert(token);

    debugPrintHeader("-");
    disassembleToken(token);
    debugPrintHeader("");
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
