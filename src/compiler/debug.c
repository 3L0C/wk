#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

/* common includes */
#include "common/debug.h"

/* local includes */
#include "debug.h"
#include "lazy_string.h"
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
disassembleLazyString(const LazyString* string, const char* title, int indent)
{
    assert(string);

    if (title == NULL) title = "(null)";

    char buffer[string->length + 1];
    lazyStringWriteToBuffer(string, buffer);
    debugMsgWithIndent(indent, "| %-20s '%s'", title, buffer);
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
    debugMsg(true, "| Interp Type:       %s", tokenLiteral(scanner->interpType));
    debugMsg(true, "|");
    debugPrintHeader("");
}

void
disassembleSingleToken(const Token* token)
{
    assert(token);

    debugPrintHeader("");
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
        token->line,
        token->column,
        "TOKEN_ERROR",
        (int)token->length,
        token->start);
}

static void
printSimpleToken(const Token* token)
{
    assert(token);

    debugMsg(
        true,
        "| %04zu:%04zu | %-27s | %-26.*s |",
        token->line,
        token->column,
        tokenLiteral(token->type),
        (int)token->length,
        token->start);
}

void
disassembleToken(const Token* token)
{
    assert(token);

    if (token->type == TOKEN_ERROR) printErrorToken(token);
    else printSimpleToken(token);
}
