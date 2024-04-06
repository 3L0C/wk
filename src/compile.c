#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "lib/common.h"
#include "lib/debug.h"
#include "lib/memory.h"
#include "lib/types.h"

#include "compile.h"
#include "debug.h"
#include "line.h"
#include "scanner.h"
#include "token.h"

static void compileLines(Chord* chords, LineArray* lines);
static size_t countStringLengthFromTokens(TokenArray* tokens, Line* line);
static char* compileStringFromTokens(TokenArray* tokens, Line* line);

static const char* delimiter;
static size_t delimLen;
static bool debug;

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

static char*
compileKey(Token* token)
{
    char* key = ALLOCATE(char, token->length + 1); /* +1 for null byte '\0' */
    memcpy(key, token->start, token->length);
    key[token->length] = '\0';

    return key;
}

static size_t
countCharactersInNumber(int num)
{
    return snprintf(NULL, 0, "%d", num);
}

static size_t
rstripToken(Token* token)
{
    size_t index = token->length - 1;
    const char* lexeme = token->start;
    while (index && isspace(lexeme[index])) index--;
    return index + 1;
}

static size_t
countStringLengthFromToken(Token* token, Line* line)
{
    switch (token->type)
    {
    case TOKEN_INDEX: return countCharactersInNumber(line->index);
    case TOKEN_INDEX_ONE: return countCharactersInNumber(line->index + 1);
    case TOKEN_THIS_DESC: return countStringLengthFromTokens(&line->description, line);
    case TOKEN_THIS_KEY:    /* FALLTHROUGH */
    case TOKEN_COMM_INTERP:
    case TOKEN_DESC_INTERP: return token->length;
    default: return rstripToken(token);
    }
}

size_t
countStringLengthFromTokens(TokenArray* tokens, Line* line)
{
    size_t result = 0;
    for (size_t i = 0; i < tokens->count; i++)
    {
        result += countStringLengthFromToken(&tokens->tokens[i], line);
    }
    return result;
}

static size_t
compileStringFromNumber(char* string, int num)
{
    size_t len = snprintf(NULL, 0, "%d", num);
    char buffer[len + 1]; /* +1 for null byte '\0' */
    snprintf(buffer, len + 1, "%d", num);
    memcpy(string, buffer, len);
    /* NOTE not copying null byte '\0'
     * returning the next byte to write to.
     */
    return len;
}

static size_t
compileStringFromToken(char* string, Token* token, Line* line)
{
    const char* source = NULL;
    size_t length = 0;
    switch (token->type)
    {
    case TOKEN_INDEX: return compileStringFromNumber(string, line->index);
    case TOKEN_INDEX_ONE: return compileStringFromNumber(string, line->index + 1);
    case TOKEN_THIS_DESC:
    {
        source = compileStringFromTokens(&line->description, line);
        length = countStringLengthFromTokens(&line->description, line);
        break;
    }
    case TOKEN_THIS_KEY:
    {
        source = line->key.start;
        length = line->key.length;
        break;
    }
    case TOKEN_COMM_INTERP: /* FALLTHROUGH */
    case TOKEN_DESC_INTERP:
    {
        source = token->start;
        length = token->length;
        break;
    }
    default: /* End of string. */
    {
        source = token->start;
        length = rstripToken(token);
        if (length == 0) return length; /* NOTE return, nothing to copy. */
        break;
    }
    }
    memcpy(string, source, length);
    return length;
}

char*
compileStringFromTokens(TokenArray* tokens, Line* line)
{
    if (tokens->count == 0)
    {
        return NULL;
    }

    size_t count = tokens->count;
    size_t size = countStringLengthFromTokens(tokens, line);
    size_t index = 0;
    char* result = NULL;

    result = ALLOCATE(char, size + 1); /* +1 for null byte '\0' */

    for (size_t i = 0; i < count; i++)
    {
        index += compileStringFromToken(&result[index], &tokens->tokens[i], line);
    }
    result[size] = '\0';

    return result;
}

static void
compileModsHint(char* hint, WkMods* mods)
{
    size_t index = 0;
    if (mods->ctrl)
    {
        hint[index++] = 'C';
        hint[index++] = '-';
    }
    if (mods->alt)
    {
        hint[index++] = 'M';
        hint[index++] = '-';
    }
    if (mods->hyper)
    {
        hint[index++] = 'H';
        hint[index++] = '-';
    }
    if (mods->shift)
    {
        hint[index++] = 'S';
        hint[index++] = '-';
    }
}

static char*
compileHintString(WkMods* mods, const char* key, const char* description)
{
    size_t modslen = COUNT_MODS(*mods) * 2;
    size_t keylen = strlen(key);
    size_t desclen = strlen(description);
    /* +1 for null byte '\0'. */
    char* hint = ALLOCATE(char, modslen + keylen + delimLen + desclen + 1);
    compileModsHint(hint, mods);
    /* Copy key. */
    memcpy(&hint[modslen], key, keylen);
    /* Copy delimiter. */
    memcpy(&hint[modslen + keylen], delimiter, delimLen);
    /* Copy description. */
    memcpy(&hint[modslen + keylen + delimLen], description, desclen);
    /* End hint. */
    hint[modslen + keylen + delimLen + desclen] = '\0';
    return hint;
}

static void
compileFlags(WkFlags* a, WkFlags* b)
{
    a->keep = b->keep;
    a->close = b->close;
    a->inherit = b->inherit;
    a->unhook = b->unhook;
    a->nobefore = b->nobefore;
    a->noafter = b->noafter;
    a->write = b->write;
    a->syncCommand = b->syncCommand;
    a->beforeAsync = b->beforeAsync;
    a->afterSync = b->afterSync;
}

static void
compileLine(Chord* chord, Line* line)
{
    compileMods(&chord->mods, &line->mods);
    compileSpecial(&chord->special, &line->key);
    chord->key = compileKey(&line->key);
    chord->description = compileStringFromTokens(&line->description, line);
    chord->hint = compileHintString(&chord->mods, chord->key, chord->description);
    chord->command = compileStringFromTokens(&line->command, line);
    chord->before = compileStringFromTokens(&line->before, line);
    chord->after = compileStringFromTokens(&line->after, line);
    compileFlags(&chord->flags, &line->flags);

    /* prefix */
    if (line->array.count)
    {
        chord->chords = ALLOCATE(Chord, line->array.count + 1); /* +1 for NULL_CHORD */
        compileLines(chord->chords, &line->array);
    }
    else
    {
        chord->chords = NULL;
    }

    if (debug)
    {
        disassembleLineShallow(line, line->index);
        debugChord(chord, 0);
    }
}

static void
compileNullChord(Chord* chord)
{
    RESET_MODS(chord->mods);
    chord->special = WK_SPECIAL_NONE;
    chord->key = NULL;
    chord->description = NULL;
    chord->hint = NULL;
    chord->command = NULL;
    chord->before = NULL;
    chord->after = NULL;
    chord->chords = NULL;
}

void
compileLines(Chord* chords, LineArray* lines)
{
    size_t count = lines->count;
    for (size_t i = 0; i < count; i++)
    {
        compileLine(&chords[i], &lines->lines[i]);
    }
    compileNullChord(&chords[count]);
}

bool
compileChords(Compiler* compiler, WkProperties* props)
{
    assert(compiler && props);

    if (compiler->lines.count == 0)
    {
        warnMsg("Nothing to compile.");
        return false;
    }

    LineArray* lines = &compiler->lines;
    Chord* chords = ALLOCATE(Chord, lines->count + 1); /* +1 for NULL_CHORD */

    delimiter = props->delimiter;
    delimLen = strlen(delimiter);
    debug = props->debug;

    compileLines(chords, lines);

    if (props->debug)
    {
        debugChords(chords, 0);
    }

    props->chords = chords;
    return true;
}

void
initCompiler(Compiler* compiler, const char* source)
{
    initScanner(&compiler->scanner, source);
    compiler->hadError      = false;
    compiler->panicMode     = false;
    compiler->index         = 0;
    initLine(&compiler->line);
    compiler->lineDest      = &compiler->lines;
    compiler->linePrefix    = NULL;
    initLineArray(&compiler->lines);
}
