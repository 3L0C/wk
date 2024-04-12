#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* common includes */
#include "common/common.h"
#include "common/debug.h"
#include "common/memory.h"
#include "common/menu.h"
#include "common/string.h"
#include "common/types.h"

/* local includes */
#include "compile.h"
#include "line.h"
#include "scanner.h"
#include "token.h"

typedef enum
{
    COMP_STATE_NORMAL,
    COMP_STATE_IGNORING,
    COMP_STATE_UPPER_FIRST,
    COMP_STATE_LOWER_FIRST,
    COMP_STATE_UPPER_ALL,
    COMP_STATE_LOWER_ALL,
} CompilerState;

typedef int (*ConversionFp)(int);

static void compileLines(WkKeyChord* keyChords, LineArray* lines);
static void compileStringFromTokens(TokenArray* tokens, Line* line, String* result);

static const char* delimiter = " -> ";
static size_t delimLen = 4;
static bool debug = false;
static CompilerState compilerState = COMP_STATE_NORMAL;

static void
compileMods(WkMods* a, WkMods* b)
{
    a->ctrl = b->ctrl;
    a->alt = b->alt;
    a->hyper = b->hyper;
    a->shift = b->shift;
}

static void
compileSpecial(WkSpecial* a, Token* token)
{
    switch (token->type)
    {
    /* Special key */
    case TOKEN_SPECIAL_LEFT:        *a = WK_SPECIAL_LEFT;       break;
    case TOKEN_SPECIAL_RIGHT:       *a = WK_SPECIAL_RIGHT;      break;
    case TOKEN_SPECIAL_UP:          *a = WK_SPECIAL_UP;         break;
    case TOKEN_SPECIAL_DOWN:        *a = WK_SPECIAL_DOWN;       break;
    case TOKEN_SPECIAL_TAB:         *a = WK_SPECIAL_TAB;        break;
    case TOKEN_SPECIAL_SPACE:       *a = WK_SPECIAL_SPACE;      break;
    case TOKEN_SPECIAL_RETURN:      *a = WK_SPECIAL_RETURN;     break;
    case TOKEN_SPECIAL_DELETE:      *a = WK_SPECIAL_DELETE;     break;
    case TOKEN_SPECIAL_ESCAPE:      *a = WK_SPECIAL_ESCAPE;     break;
    case TOKEN_SPECIAL_HOME:        *a = WK_SPECIAL_HOME;       break;
    case TOKEN_SPECIAL_PAGE_UP:     *a = WK_SPECIAL_PAGE_UP;    break;
    case TOKEN_SPECIAL_PAGE_DOWN:   *a = WK_SPECIAL_PAGE_DOWN;  break;
    case TOKEN_SPECIAL_END:         *a = WK_SPECIAL_END;        break;
    case TOKEN_SPECIAL_BEGIN:       *a = WK_SPECIAL_BEGIN;      break;
    /* Regular key */
    default: *a = WK_SPECIAL_NONE; break;
    }
}

static void
compileKey(Token* token, String* result, WkKeyChord* keyChord)
{
    assert(token && result && keyChord);

    appendToString(result, token->start, token->length);
    keyChord->key = result->string;
    disownString(result);
}

static size_t
rstripToken(Token* token)
{
    size_t index = token->length - 1;
    const char* lexeme = token->start;
    while (index && isspace(lexeme[index])) index--;
    return index + 1;
}

static void
compileStringFromNumber(String* string, uint32_t num)
{
    appendInt32ToString(string, num);
}

static void
compileDescriptionWithState(TokenArray* tokens, Line* line, String* result, CompilerState state)
{
    CompilerState oldState = compilerState;
    compilerState = state;
    compileStringFromTokens(tokens, line, result);
    compilerState = oldState;
}

static void
compileCharsToStringWithFp(String* ressult, const char* source, size_t len, ConversionFp fp)
{
    for (size_t i = 0; i < len; i++)
    {
        appendCharToString(ressult, fp(source[i]));
    }
}

static void
compileToStringWithState(String* result, const char* source, size_t len)
{
    assert(result && source);

    if (len < 1) return;

    switch (compilerState)
    {
    case COMP_STATE_UPPER_FIRST:
    {
        compilerState = COMP_STATE_IGNORING;
        appendCharToString(result, toupper(*source));
        appendToString(result, source + 1, len - 1);
        break;
    }
    case COMP_STATE_LOWER_FIRST:
    {
        compilerState = COMP_STATE_IGNORING;
        appendCharToString(result, tolower(*source));
        appendToString(result, source + 1, len - 1);
        break;
    }
    case COMP_STATE_UPPER_ALL: compileCharsToStringWithFp(result, source, len, toupper); break;
    case COMP_STATE_LOWER_ALL: compileCharsToStringWithFp(result, source, len, tolower); break;
    default: appendToString(result, source, len); break;
    }
}

static void
compileStringFromToken(Token* token, Line* line, String* result)
{
    switch (token->type)
    {
    case TOKEN_INDEX: compileStringFromNumber(result, line->index); break;
    case TOKEN_INDEX_ONE: compileStringFromNumber(result, line->index + 1); break;
    case TOKEN_THIS_DESC: compileStringFromTokens(&line->description, line, result); break;
    case TOKEN_THIS_DESC_UPPER_FIRST:
    {
        compileDescriptionWithState(&line->description, line, result, COMP_STATE_UPPER_FIRST);
        break;
    }
    case TOKEN_THIS_DESC_LOWER_FIRST:
    {
        compileDescriptionWithState(&line->description, line, result, COMP_STATE_LOWER_FIRST);
        break;
    }
    case TOKEN_THIS_DESC_UPPER_ALL:
    {
        compileDescriptionWithState(&line->description, line, result, COMP_STATE_UPPER_ALL);
        break;
    }
    case TOKEN_THIS_DESC_LOWER_ALL:
    {
        compileDescriptionWithState(&line->description, line, result, COMP_STATE_LOWER_ALL);
        break;
    }
    case TOKEN_THIS_KEY: appendToString(result, line->key.start, line->key.length); break;
    case TOKEN_COMM_INTERP: /* FALLTHROUGH */
    case TOKEN_DESC_INTERP: compileToStringWithState(result, token->start, token->length); break;
    default: /* End of string, i.e. TOKEN_COMMAND and TOKEN_DESCRIPTION */
    {
        size_t length = rstripToken(token);
        if (length == 0) return; /* NOTE return, nothing to copy. */
        compileToStringWithState(result, token->start, length);
        break;
    }
    }
}

void
compileStringFromTokens(TokenArray* tokens, Line* line, String* result)
{
    assert(tokens && line && result);

    /* Nothing to compile */
    if (tokens->count == 0) return;

    size_t tokenCount = tokens->count;
    for (size_t i = 0; i < tokenCount; i++)
    {
        compileStringFromToken(tokens->tokens + i, line, result);
    }
}

static void
compileDescription(Line* line, String* result, WkKeyChord* keyChord)
{
    assert(line && result && keyChord);

    compileStringFromTokens(&line->description, line, result);
    keyChord->description = result->string;
    disownString(result);
}



static void
compileModsHint(WkMods* mods, String* result)
{
    assert(mods && result);

    if (mods->ctrl)  appendToString(result, "C-", 2);
    if (mods->alt)   appendToString(result, "M-", 2);
    if (mods->hyper) appendToString(result, "H-", 2);
    if (mods->shift) appendToString(result, "S-", 2);
}

static void
compileHintString(WkMods* mods, const char* key, const char* description, String* result)
{
    compileModsHint(mods, result);
    appendToString(result, key, strlen(key));
    appendToString(result, delimiter, delimLen);
    appendToString(result, description, strlen(description));
}

static void
compileHint(String* result, WkKeyChord* keyChord)
{
    assert(result && keyChord);

    compileHintString(&keyChord->mods, keyChord->key, keyChord->description, result);
    keyChord->hint = result->string;
    disownString(result);

}

static void
compileCommand(Line* line, String* result, WkKeyChord* keyChord)
{
    assert(line && result && keyChord);

    compileStringFromTokens(&line->command, line, result);
    keyChord->command = result->string;
    disownString(result);
}

static void
compileBeforeCommand(Line* line, String* result, WkKeyChord* keyChord)
{
    assert(line && result && keyChord);

    compileStringFromTokens(&line->before, line, result);
    keyChord->before = result->string;
    disownString(result);
}

static void
compileAfterCommand(Line* line, String* result, WkKeyChord* keyChord)
{
    assert(line && result && keyChord);

    compileStringFromTokens(&line->after, line, result);
    keyChord->after = result->string;
    disownString(result);
}

static void
compileFlags(WkFlags* from, WkFlags* to)
{
    COPY_FLAGS(*from, *to);
}

static void
compileLine(WkKeyChord* keyChord, Line* line)
{
    assert(keyChord && line);

    compileMods(&keyChord->mods, &line->mods);
    compileSpecial(&keyChord->special, &line->key);

    String result = {0};
    initString(&result);

    compileKey(&line->key, &result, keyChord);
    compileDescription(line, &result, keyChord);
    compileHint(&result, keyChord);
    compileCommand(line, &result, keyChord);
    compileBeforeCommand(line, &result, keyChord);
    compileAfterCommand(line, &result, keyChord);
    compileFlags(&line->flags, &keyChord->flags);

    /* prefix */
    if (line->array.count)
    {
        keyChord->keyChords = ALLOCATE(WkKeyChord, line->array.count + 1); /* +1 for NULL_CHORD */
        compileLines(keyChord->keyChords, &line->array);
    }
    else
    {
        keyChord->keyChords = NULL;
    }
}

static void
compileNullKeyChord(WkKeyChord* keyChord)
{
    RESET_MODS(keyChord->mods);
    keyChord->special = WK_SPECIAL_NONE;
    keyChord->key = NULL;
    keyChord->description = NULL;
    keyChord->hint = NULL;
    keyChord->command = NULL;
    keyChord->before = NULL;
    keyChord->after = NULL;
    keyChord->keyChords = NULL;
}

void
compileLines(WkKeyChord* keyChords, LineArray* lines)
{
    size_t count = lines->count;
    for (size_t i = 0; i < count; i++)
    {
        compileLine(&keyChords[i], &lines->lines[i]);
    }
    compileNullKeyChord(&keyChords[count]);
}

bool
compileKeyChords(Compiler* compiler, WkMenu* menu)
{
    assert(compiler && menu);

    compilerState = COMP_STATE_NORMAL;

    if (compiler->lines.count == 0)
    {
        warnMsg("Nothing to compile.");
        return false;
    }

    LineArray* lines = &compiler->lines;
    WkKeyChord* keyChords = ALLOCATE(WkKeyChord, lines->count + 1); /* +1 for NULL_CHORD */

    delimiter = menu->delimiter;
    delimLen = strlen(delimiter);
    debug = menu->debug;

    compileLines(keyChords, lines);

    if (menu->debug)
    {
        debugKeyChords(keyChords, 0);
    }

    menu->keyChords = keyChords;
    return true;
}

void
initCompiler(Compiler* compiler, const char* source, const char* filePath)
{
    initScanner(&compiler->scanner, source, filePath);
    compiler->hadError = false;
    compiler->panicMode = false;
    compiler->index = 0;
    initLine(&compiler->line);
    compiler->lineDest = &compiler->lines;
    compiler->linePrefix = NULL;
    initLineArray(&compiler->lines);
}
