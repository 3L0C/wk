#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "lib/common.h"
#include "lib/types.h"

#include "debug.h"
#include "line.h"
#include "scanner.h"
#include "token.h"

static void
printErrorToken(Token* token)
{
    printf("[%04zu] Error:      \"%s\".\n", token->line, token->message);
    printf("    |  Lexeme:     '%.*s'\n", (int)token->length, token->start);
}

static void
printSimpleToken(Token* token, const char* type)
{
    printf("[%04zu] TokenType: %-20s", token->line, type);
    printf(" |  Lexeme: '%.*s'\n", (int)token->length, token->start);
}

void
disassembleToken(Token* token)
{
    assert(token);

    char* type = "";
    switch (token->type)
    {
    /* single characters */
    case TOKEN_LEFT_BRACKET:        type = "TOKEN_LEFT_BRACKET";        break;
    case TOKEN_RIGHT_BRACKET:       type = "TOKEN_RIGHT_BRACKET";       break;
    case TOKEN_LEFT_BRACE:          type = "TOKEN_LEFT_BRACE";          break;
    case TOKEN_RIGHT_BRACE:         type = "TOKEN_RIGHT_BRACE";         break;

    /* keywords */
    case TOKEN_INDEX:               type = "TOKEN_INDEX";               break;
    case TOKEN_INDEX_ONE:           type = "TOKEN_INDEX_ONE";           break;
    case TOKEN_THIS_KEY:            type = "TOKEN_THIS_KEY";            break;
    case TOKEN_BEFORE:              type = "TOKEN_BEGIN";               break;
    case TOKEN_AFTER:               type = "TOKEN_BEGIN";               break;
    case TOKEN_KEEP:                type = "TOKEN_KEEP";                break;
    case TOKEN_INHERIT:             type = "TOKEN_INHERIT";             break;
    case TOKEN_UNHOOK:              type = "TOKEN_UNHOOK";              break;
    case TOKEN_NO_BEFORE:           type = "TOKEN_NO_BEFORE";           break;
    case TOKEN_NO_AFTER:            type = "TOKEN_NO_AFTER";            break;

    /* literals */
    case TOKEN_COMMAND:             type = "TOKEN_COMMAND";             break;
    case TOKEN_COMM_INTERP:         type = "TOKEN_COMM_INTERP";         break;
    case TOKEN_DESCRIPTION:         type = "TOKEN_DESCRIPTION";         break;
    case TOKEN_DESC_INTERP:         type = "TOKEN_DESC_INTERP";         break;

    /* keys */
    case TOKEN_KEY:                 type = "TOKEN_KEY";                 break;

    /* mods */
    case TOKEN_MOD_CTRL:            type = "TOKEN_MOD_CTRL";            break;
    case TOKEN_MOD_ALT:             type = "TOKEN_MOD_ALT";             break;
    case TOKEN_MOD_HYPER:           type = "TOKEN_MOD_HYPER";           break;
    case TOKEN_MOD_SHIFT:           type = "TOKEN_MOD_SHIFT";           break;

    /* specials */
    case TOKEN_SPECIAL_LEFT:        type = "TOKEN_SPECIAL_LEFT";        break;
    case TOKEN_SPECIAL_RIGHT:       type = "TOKEN_SPECIAL_RIGHT";       break;
    case TOKEN_SPECIAL_UP:          type = "TOKEN_SPECIAL_UP";          break;
    case TOKEN_SPECIAL_DOWN:        type = "TOKEN_SPECIAL_DOWN";        break;
    case TOKEN_SPECIAL_TAB:         type = "TOKEN_SPECIAL_TAB";         break;
    case TOKEN_SPECIAL_SPACE:       type = "TOKEN_SPECIAL_SPACE";       break;
    case TOKEN_SPECIAL_RETURN:      type = "TOKEN_SPECIAL_RETURN";      break;
    case TOKEN_SPECIAL_DELETE:      type = "TOKEN_SPECIAL_DELETE";      break;
    case TOKEN_SPECIAL_ESCAPE:      type = "TOKEN_SPECIAL_ESCAPE";      break;
    case TOKEN_SPECIAL_HOME:        type = "TOKEN_SPECIAL_HOME";        break;
    case TOKEN_SPECIAL_PAGE_UP:     type = "TOKEN_SPECIAL_PAGE_UP";     break;
    case TOKEN_SPECIAL_PAGE_DOWN:   type = "TOKEN_SPECIAL_PAGE_DOWN";   break;
    case TOKEN_SPECIAL_END:         type = "TOKEN_SPECIAL_END";         break;
    case TOKEN_SPECIAL_BEGIN:       type = "TOKEN_SPECIAL_BEGIN";       break;

    /* control */
    case TOKEN_EOF:                                     return;
    case TOKEN_ERROR:       printErrorToken(token);     return;
    case TOKEN_NO_INTERP:   type = "TOKEN_NO_INTERP";   break;

    default: type = "UNKNOWN TYPE!"; break;
    }
    printSimpleToken(token, type);
}

void
debugScanner(const char* source)
{
    assert(source);

    Scanner scanner;
    initScanner(&scanner, source);
    while (true)
    {
        Token token = scanToken(&scanner);
        if (token.type == TOKEN_EOF) return;
        disassembleToken(&token);
    }
}

static void
printMods(int mod)
{
    printf("       ");
    if (!IS_MOD(mod))
    {
        printf("Mods: WK_MOD_NONE\n");
        return;
    }

    printf("Mods: ");
    const char* mods[4];
    int idx = 0;
    if (IS_CTRL(mod)) mods[idx++] = "WK_MOD_CTRL";
    if (IS_ALT(mod)) mods[idx++] = "WK_MOD_ALT";
    if (IS_HYPER(mod)) mods[idx++] = "WK_MOD_HYPER";
    if (IS_SHIFT(mod)) mods[idx++] = "WK_MOD_SHIFT";

    for (int i = 0; i < idx; i++)
    {
        printf("%s%s", mods[i], (i + 1 == idx) ? "\n" : "|");
    }
}

static void
printToken(Token token, const char* message)
{
    printf("       ");
    printf("%s: '%.*s'", message, (int)token.length, token.start);
    printf("\n");
}

static void
printTokenArray(Line* line, TokenArray* array, const char* message)
{
    printf("       ");
    printf("%s: ", message);
    if (array->count == 0) printf("None");
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
    printf("\n");
}

static void
printFlag(WkFlag flags, WkFlag flag, const char* repr, const char delim)
{
    if (IS_FLAG(flags, flag)) printf("%s%c", repr, delim);
}

static void
printFlags(WkFlag flags)
{
    printf("       ");
    if (flags == 0)
    {
        printf("Flags: WK_FLAG_DEFAULTS|%d\n", flags);
        return;
    }

    printf("Flags: ");
    printFlag(flags, WK_FLAG_KEEP, "WK_FLAG_KEEP", '|');
    printFlag(flags, WK_FLAG_INHERIT, "WK_FLAG_INHERIT", '|');
    printFlag(flags, WK_FLAG_UNHOOK, "WK_FLAG_UNHOOK", '|');
    printFlag(flags, WK_FLAG_NOBEFORE, "WK_FLAG_NOBEFORE", '|');
    printFlag(flags, WK_FLAG_NOAFTER, "WK_FLAG_NOAFTER", '|');
    printFlag(flags, WK_FLAG_WRITE, "WK_FLAG_WRITE", '|');
    printFlag(flags, WK_FLAG_SYNC_COMMAND, "WK_FLAG_SYNC_COMMAND", '|');
    printFlag(flags, WK_FLAG_BEFORE_ASYNC, "WK_FLAG_BEFORE_ASYNC", '|');
    printFlag(flags, WK_FLAG_AFTER_SYNC, "WK_FLAG_AFTER_SYNC", '|');
    printf("%d\n", flags);
}

void
disassembleLine(Line* line, size_t index)
{
    assert(line);

    printf("[%04zu] Line\n", index);
    printf("       Index: %04d\n", line->index);
    printFlags(line->flags);
    printMods(line->mods);
    printToken(line->key, "Key");
    printTokenArray(line, &line->description, "Description");
    printTokenArray(line, &line->command, "Command");
    printTokenArray(line, &line->before, "Before");
    printTokenArray(line, &line->after, "After");
    debugLineArray(&line->array);
}

void
debugLineArray(LineArray* array)
{
    assert(array);

    for (size_t i = 0; i < array->count; i++)
    {
        disassembleLine(&array->lines[i], i);
    }
}
