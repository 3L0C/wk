#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

/* common includes */
#include "common/debug.h"

/* local includes */
#include "debug.h"
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
    debugPrintHeader("");
    debugMsg(true, "|                            Scanned Tokens                            |");
    debugPrintHeader("");
    debugMsg(true, "| Line:Col  |          TokenType          |           Lexeme           |");
    debugPrintHeader("");
    debugMsg(true, "|           |                             |                            |");
}

void
disassembleArrayAsText(const Array* arr, const char* title)
{
    assert(arr), assert(title);

    debugPrintHeader(title);
    debugMsg(true, "| ");
    debugTextWithLineNumber(ARRAY_AS(arr, char));
    debugMsg(true, "| ");
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
    debugMsg(true, "| Interp Type:       %s", tokenGetLiteral(scanner->interpType));
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
        token->line, token->column, tokenGetLiteral(token->type),
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
