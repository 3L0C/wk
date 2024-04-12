#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* common includes */
#include "common/types.h"
#include "common/debug.h"

/* local includes */
#include "debug.h"
#include "line.h"
#include "piece_table.h"
#include "token.h"

static void
debugWithIndent(int indent, const char* fmt, ...)
{
    assert(fmt);

    printf("[DEBUG] ");
    for (int i = 0; i < indent; i++)
    {
        printf("|      ");
    }

    size_t len = strlen(fmt);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    if (fmt[len - 1] != ' ')
    {
        fputc('\n', stdout);
    }
}

static char
getDelim(int* count, char a, char b)
{
    return ((*count)-- > 1 ? a : b);
}

static void
disassembleMods(WkMods* mods, int indent)
{
    if (!IS_MOD(*mods))
    {
        debugWithIndent(indent, "| Mods:        NONE");
        return;
    }

    int count = COUNT_MODS(*mods);
    debugWithIndent(indent, "| Mods:        ");
    if (mods->ctrl) printf("CTRL%c", getDelim(&count, '|', '\n'));
    if (mods->alt) printf("ALT%c", getDelim(&count, '|', '\n'));
    if (mods->hyper) printf("HYPER%c", getDelim(&count, '|', '\n'));
    if (mods->shift) printf("SHIFT%c", getDelim(&count, '|', '\n'));
}

static void
disassembleLineToken(Token* token, const char* message, int indent)
{
    debugWithIndent(indent, "| %s:         '%.*s'", message, (int)token->length, token->start);
}

static void
disassembleTokenArray(Line* line, TokenArray* array, const char* message, int indent)
{
    assert(line && array && message);

    if (array->count == 0)
    {
        debugWithIndent(indent, "| %s %s", message, "NONE");
        return;
    }

    debugWithIndent(indent, "| %s ", message);
    printf("\"");
    for (size_t i = 0; i < array->count; i++)
    {
        Token* token = &array->tokens[i];
        switch (token->type)
        {
        case TOKEN_INDEX:       printf("%d", line->index); break;
        case TOKEN_INDEX_ONE:   printf("%d", line->index + 1); break;
        case TOKEN_THIS_KEY:    printf("%.*s", (int)line->key.length, line->key.start); break;
        default: printf("%.*s", (int)token->length, token->start); break;
        }
    }
    printf("\"\n");
}

static void
disassembleFlags(WkFlags* flags, int indent)
{
    if (!HAS_FLAG(*flags))
    {
        debugWithIndent(indent, "| Flags:       WK_FLAG_DEFAULTS");
        return;
    }

    int count = COUNT_FLAGS(*flags);
    debugWithIndent(indent, "| Flags:       ");
    if (flags->keep) printf("KEEP%c", getDelim(&count, '|', '\n'));
    if (flags->close) printf("CLOSE%c", getDelim(&count, '|', '\n'));
    if (flags->inherit) printf("INHERIT%c", getDelim(&count, '|', '\n'));
    if (flags->ignore) printf("UNHOOK%c", getDelim(&count, '|', '\n'));
    if (flags->nobefore) printf("NOBEFORE%c", getDelim(&count, '|', '\n'));
    if (flags->noafter) printf("NOAFTER%c", getDelim(&count, '|', '\n'));
    if (flags->write) printf("WRITE%c", getDelim(&count, '|', '\n'));
    if (flags->syncCommand) printf("SYNC_COMMAND%c", getDelim(&count, '|', '\n'));
    if (flags->beforeSync) printf("BEFORE_SYNC%c", getDelim(&count, '|', '\n'));
    if (flags->afterSync) printf("AFTER_SYNC%c", getDelim(&count, '|', '\n'));
}

/*
[DEBUG] | Total lines: 3
[DEBUG] |
[DEBUG] | Line number: 0000
[DEBUG] | Mods:
[DEBUG] | Key:
[DEBUG] | Description:
[DEBUG] | ...
[DEBUG] | Lines:
[DEBUG] |      |
[DEBUG] |      | Total lines: 4
[DEBUG] |      |------------------
[DEBUG] |      | Line number: 0000
[DEBUG] |      | Lines:
[DEBUG] |      |      |
[DEBUG] |      |      | Total lines: 4
[DEBUG] |
 *
 */

/*
struct LineArray
{
    Line*  lines;
    size_t count;
    size_t capacity;
};

struct Line
{
    uint32_t    index;
    WkMods      mods;
    Token       key;
    TokenArray  description;
    TokenArray  command;
    TokenArray  before;
    TokenArray  after;
    WkFlags     flags;
    LineArray   array;
};

 *
 */
void
disassembleLine(Line* line, size_t index, int indent)
{
    assert(line);

    debugWithIndent(indent, "|");
    debugWithIndent(indent, "| Line number: %04zu", line->index);
    disassembleMods(&line->mods, indent);
    disassembleLineToken(&line->key, "Key", indent);
    disassembleFlags(&line->flags, indent);
    disassembleTokenArray(line, &line->description, "Description:", indent);
    disassembleTokenArray(line, &line->command,     "Command:    ", indent);
    disassembleTokenArray(line, &line->before,      "Before:     ", indent);
    disassembleTokenArray(line, &line->after,       "After:      ", indent);
    if (line->array.count)
    {
        debugWithIndent(indent, "|");
        debugWithIndent(indent, "|------- Nested Lines: %04zu -------", line->array.count);
        disassembleLineArray(&line->array, indent + 1);
    }
    else
    {
        debugWithIndent(indent, "| Lines:       (null)");
        debugWithIndent(indent, "|");
    }
    debugWithIndent(indent, "----------------------------");
}

void
disassembleLineArray(LineArray* array, int indent)
{
    assert(array);

    if (indent == 0) debugPrintHeader("LineArray");
    for (size_t i = 0; i < array->count; i++)
    {
        disassembleLine(&array->lines[i], i, indent);
    }
    if (indent == 0) printf("\n");
}

void
disassembleLineShallow(Line* line, size_t index)
{
    assert(line);

    debugWithIndent(0, "Line number: %04zu", line->index);
    disassembleMods(&line->mods, 0);
    disassembleLineToken(&line->key, "Key", 0);
    disassembleFlags(&line->flags, 0);
    disassembleTokenArray(line, &line->description, "Description:", 0);
    disassembleTokenArray(line, &line->command,     "Command:    ", 0);
    disassembleTokenArray(line, &line->before,      "Before:     ", 0);
    disassembleTokenArray(line, &line->after,       "After:      ", 0);
}

void
disassemblePiece(PieceTable* pieceTable, size_t index)
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
    debugMsg(true, "| Text: ");
    debugMsg(true, "| ");
    debugStringLenWithIndent(getTextAtPiece(pieceTable, piece), piece->len);
}

static void
disassemblePieceArray(PieceTable* pieceTable)
{
    assert(pieceTable);

    PieceArray* array = &pieceTable->pieces;
    for (size_t i = 0; i < array->count; i++)
    {
        debugMsg(true, "|----------- Piece number: %04zu ------------", i);
        disassemblePiece(pieceTable, i);
        debugMsg(true, "| ");
    }
}

void
disassemblePieceTable(PieceTable* pieceTable)
{
    assert(pieceTable);

    debugMsg(true, "---------------- PieceTable ----------------");
    debugMsg(true, "| ");
    debugMsg(true, "| Total pieces:         %zu", pieceTable->pieces.count);
    debugMsg(true, "| Original Text length: %zu", pieceTable->originalLen);
    debugMsg(true, "| Add Text length:      %zu", pieceTable->add.count);
    debugMsg(true, "| ");
    debugMsg(true, "|-------------- Original Text --------------");
    debugMsg(true, "| ");
    debugStringWithIndent(pieceTable->original);
    debugMsg(true, "| ");
    if (pieceTable->add.string)
    {
        debugMsg(true, "|---------------- Add Text -----------------");
        debugMsg(true, "| ");
        debugStringWithIndent(pieceTable->add.string);
    }
    debugMsg(true, "| ");
    disassemblePieceArray(pieceTable);
    debugMsg(true, "--------------------------------------------\n");
}

static void
printErrorToken(Token* token)
{
    printf("[%04zu:%04zu] Error:      \"%s\".\n", token->line, token->column, token->message);
    printf("    |  Lexeme:     '%.*s'\n", (int)token->length, token->start);
}

static void
printSimpleToken(Token* token, const char* type)
{
    debugMsg(
        true,
        "| %04zu:%04zu | %-27s | '%.*s'",
        token->line, token->column, type,
        (int)token->length, token->start
    );
}

void
disassembleToken(Token* token)
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

    default: type = "UNKNOWN TYPE!"; break;
    }
    printSimpleToken(token, type);
}

