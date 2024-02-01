#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/common.h"
#include "lib/memory.h"
#include "lib/types.h"

#include "debug.h"
#include "line.h"
#include "scanner.h"
#include "compile.h"
#include "token.h"
#include "transpiler.h"
#include "writer.h"

static void keyChord(Compiler* compiler);

static bool debug;

static LineArray*
nextLineArray(Compiler* compiler)
{
    return &compiler->lineDest->lines[compiler->lineDest->count - 1].array;
}

static void
errorAt(Compiler* compiler, Token* token, const char* message)
{
    if (compiler->panicMode) return;
    compiler->panicMode = true;

    fprintf(stderr, "[ERROR] [line %04zu] Error", token->line);

    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR)
    {
        fprintf(stderr, " at line %04zu", token->line);
    }
    else
    {
        fprintf(stderr, " at '%.*s'", (int)token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    compiler->hadError = true;
}

static void
error(Compiler* compiler, const char* message)
{
    errorAt(compiler, &compiler->previous, message);
}

static void
errorAtCurrent(Compiler* compiler, const char* message)
{
    errorAt(compiler, &compiler->current, message);
}

static void
advance(Compiler* compiler)
{
    compiler->previous = compiler->current;

    while (true)
    {
        compiler->current = scanToken(&compiler->scanner);
        if (debug) disassembleToken(&compiler->current);
        if (compiler->current.type != TOKEN_ERROR) break;

        errorAtCurrent(compiler, compiler->current.message);
    }
}

static void
consume(Compiler* compiler, TokenType type, const char* message)
{
    if (compiler->current.type == type)
    {
        advance(compiler);
        return;
    }

    errorAtCurrent(compiler, message);
}

static bool
check(Compiler* compiler, TokenType type)
{
    return compiler->current.type == type;
}

static bool
match(Compiler* compiler, TokenType type)
{
    if (!check(compiler, type)) return false;
    advance(compiler);
    return true;
}

static TokenType
currentType(Compiler* compiler)
{
    return compiler->current.type;
}

static Token
currentToken(Compiler* compiler)
{
    return compiler->current;
}

static bool
isMod(TokenType type)
{
    switch (type)
    {
    case TOKEN_MOD_ALT:
    case TOKEN_MOD_CTRL:
    case TOKEN_MOD_HYPER:
    case TOKEN_MOD_SHIFT: return true;
    default: return false;
    }
}

static void
addMod(TokenType type, WkMods* mods)
{
    switch (type)
    {
    case TOKEN_MOD_ALT:     mods->alt = true;    break;
    case TOKEN_MOD_CTRL:    mods->ctrl = true;   break;
    case TOKEN_MOD_HYPER:   mods->hyper = true;  break;
    case TOKEN_MOD_SHIFT:   mods->shift = true;  break;
    default: break;
    }
}

static void
mods(Compiler* compiler)
{
    if (compiler->panicMode) return;
    while (isMod(currentType(compiler)))
    {
        addMod(currentType(compiler), &compiler->line.mods);
        advance(compiler);
    }
}

static bool
isKey(Compiler* compiler)
{
    switch (currentType(compiler))
    {
    /* is a key */
    case TOKEN_KEY:                 /* FALLTHROUGH */
    case TOKEN_SPECIAL_LEFT:
    case TOKEN_SPECIAL_RIGHT:
    case TOKEN_SPECIAL_UP:
    case TOKEN_SPECIAL_DOWN:
    case TOKEN_SPECIAL_TAB:
    case TOKEN_SPECIAL_SPACE:
    case TOKEN_SPECIAL_RETURN:
    case TOKEN_SPECIAL_DELETE:
    case TOKEN_SPECIAL_ESCAPE:
    case TOKEN_SPECIAL_HOME:
    case TOKEN_SPECIAL_PAGE_UP:
    case TOKEN_SPECIAL_PAGE_DOWN:
    case TOKEN_SPECIAL_END:
    case TOKEN_SPECIAL_BEGIN:   return true;
    /* not a key */
    default: return false;
    }
}

static void
key(Compiler* compiler)
{
    if (compiler->panicMode) return;
    if (!isKey(compiler))
    {
         errorAtCurrent(compiler, "Expected key or special.");
         return;
    }
    compiler->line.key = currentToken(compiler);
    advance(compiler); /* consume the key */
}

static void
description(Compiler* compiler, TokenArray* dest)
{
    if (compiler->panicMode) return;
    if (!check(compiler, TOKEN_DESC_INTERP) &&
        !check(compiler, TOKEN_DESCRIPTION))
    {
        errorAtCurrent(compiler, "Expected description.");
        return;
    }
    while (!check(compiler, TOKEN_EOF))
    {
        switch (currentType(compiler))
        {
        case TOKEN_DESC_INTERP: /* FALLTHROUGH */
        case TOKEN_DESCRIPTION:
        case TOKEN_THIS_KEY:
        case TOKEN_THIS_DESC:
        case TOKEN_INDEX:
        case TOKEN_INDEX_ONE:
        {
            Token token = currentToken(compiler);
            writeTokenArray(dest, &token);
            break;
        }
        default: errorAtCurrent(compiler, "Malformed description."); return;
        }
        if (check(compiler, TOKEN_DESCRIPTION)) break;
        advance(compiler);
    }
    consume(compiler, TOKEN_DESCRIPTION, "Expected description.");
}

static void
command(Compiler* compiler, TokenArray* dest)
{
    if (compiler->panicMode) return;
    if (!check(compiler, TOKEN_COMM_INTERP) &&
        !check(compiler, TOKEN_COMMAND))
    {
        errorAtCurrent(compiler, "Expected command.");
        return;
    }
    while (!check(compiler, TOKEN_EOF))
    {
        switch (currentType(compiler))
        {
        case TOKEN_COMM_INTERP: /* FALLTHROUGH */
        case TOKEN_COMMAND:
        case TOKEN_THIS_KEY:
        case TOKEN_THIS_DESC:
        case TOKEN_INDEX:
        case TOKEN_INDEX_ONE:
        {
            Token token = currentToken(compiler);
            writeTokenArray(dest, &token);
            break;
        }
        default: errorAtCurrent(compiler, "Malformed command."); return;
        }
        if (check(compiler, TOKEN_COMMAND)) break;
        advance(compiler);
    }
    consume(compiler, TOKEN_COMMAND, "Expected command.");
}

static void
keywords(Compiler* compiler)
{
    if (compiler->panicMode) return;
    while (!check(compiler, TOKEN_EOF))
    {
        TokenType type = currentType(compiler);
        switch (type)
        {
        case TOKEN_BEFORE:
            consume(compiler, TOKEN_BEFORE, "Expected 'before' keyword.");
            command(compiler, &compiler->line.before);
            break;
        case TOKEN_ASYNC_BEFORE:
            consume(compiler, TOKEN_ASYNC_BEFORE, "Expect 'async-before' keyword.");
            command(compiler, &compiler->line.before);
            compiler->line.flags.beforeAsync = true;
            break;
        case TOKEN_AFTER:
            consume(compiler, TOKEN_AFTER, "Expected 'after' keyword.");
            command(compiler, &compiler->line.after);
            break;
        case TOKEN_SYNC_AFTER:
            consume(compiler, TOKEN_SYNC_AFTER, "Expect 'sync-after' keyword.");
            command(compiler, &compiler->line.after);
            compiler->line.flags.afterSync = true;
            break;
        case TOKEN_KEEP:            compiler->line.flags.keep = true; break;
        case TOKEN_INHERIT:         compiler->line.flags.inherit = true; break;
        case TOKEN_UNHOOK:          compiler->line.flags.unhook = true; break;
        case TOKEN_NO_BEFORE:       compiler->line.flags.nobefore = true; break;
        case TOKEN_NO_AFTER:        compiler->line.flags.noafter = true; break;
        case TOKEN_WRITE:           compiler->line.flags.write = true; break;
        case TOKEN_SYNC_CMD:        compiler->line.flags.syncCommand = true; break;
        default: return; /* not a keyword but not an error */
        }
        /* consume keyword */
        if (type != TOKEN_BEFORE && type != TOKEN_AFTER) advance(compiler);
    }
}

static void
chord(Compiler* compiler)
{
    initLine(&compiler->line);
    compiler->line.index = compiler->index++;

    mods(compiler);
    key(compiler);
    description(compiler, &compiler->line.description);
    keywords(compiler);

    /* check for prefix */
    if (check(compiler, TOKEN_LEFT_BRACE)) return;

    command(compiler, &compiler->line.command);

    writeLineArray(compiler->lineDest, &compiler->line);
    freeLine(&compiler->line);
}

static void
getKeys(Compiler* compiler, TokenArray* keys)
{
    if (!isKey(compiler) && !isMod(currentType(compiler)))
    {
        errorAtCurrent(compiler, "Expect key or modifier after '['");
        return;
    }
    while (isKey(compiler) || isMod(currentType(compiler)))
    {
        Token token = currentToken(compiler);
        writeTokenArray(keys, &token);
        advance(compiler);
    }
}

static bool
isCommand(Compiler* compiler)
{
    switch (currentType(compiler))
    {
    case TOKEN_COMM_INTERP: /* FALLTHROUGH */
    case TOKEN_COMMAND:
    case TOKEN_THIS_KEY:
    case TOKEN_THIS_DESC:
    case TOKEN_INDEX:
    case TOKEN_INDEX_ONE: return true;
    default: return false;
    }
}

static void
chordArray(Compiler* compiler)
{
    TokenArray keys;
    initTokenArray(&keys);
    getKeys(compiler, &keys);
    consume(compiler, TOKEN_RIGHT_BRACKET, "Expect ']' after key list.");
    description(compiler, &compiler->line.description);
    keywords(compiler);
    if (!isCommand(compiler))
    {
        error(compiler, "Expect command.");
        return;
    }
    command(compiler, &compiler->line.command);
    size_t mods = 0;
    for (size_t i = 0; i < keys.count; i++)
    {
        Line line;
        initLine(&line);
        copyLine(&compiler->line, &line);
        line.index = i - mods;
        while (isMod(keys.tokens[i].type))
        {
            addMod(keys.tokens[i].type, &line.mods);
            i++;
            mods++;
        }
        line.key = keys.tokens[i];
        writeLineArray(compiler->lineDest, &line);
        freeLine(&line);
    }
    freeLine(&compiler->line);
    FREE_ARRAY(TokenArray, keys.tokens, keys.capacity);
}

static void
setFlags(WkFlags* parent, WkFlags* child)
{
    assert(parent && child);

    if (parent->keep && !child->close) child->keep = parent->keep;
    if (parent->write) child->write = parent->write;
    if (parent->syncCommand) child->syncCommand = parent->syncCommand;
    if (parent->beforeAsync) child->beforeAsync = parent->beforeAsync;
    if (parent->afterSync) child->afterSync = parent->afterSync;
}

static void
setBeforeHook(Line* parent, Line* child)
{
    assert(parent && child);

    if (parent->before.count == 0) return;
    if (child->before.count) return;
    freeTokenArray(&child->before);
    copyTokenArray(&parent->before, &child->before);
}

static void
setAfterHook(Line* parent, Line* child)
{
    assert(parent && child);

    if (parent->after.count == 0) return;
    if (child->after.count) return;
    freeTokenArray(&child->after);
    copyTokenArray(&parent->after, &child->after);
}

static Line*
getLastLine(LineArray* lines)
{
    if (lines->count == 0) return NULL;
    return &lines->lines[lines->count - 1];
}

static void
setHooksAndFlags(Line* parent, LineArray* children)
{
    if (!parent || !children) return;

    for (size_t i = 0; i < children->count; i++)
    {
        Line* child = &children->lines[i];

        /* NOTE Don't copy anything from parent if unhooked */
        if (child->flags.unhook) continue;

        setFlags(&parent->flags, &child->flags);
        if (child->array.count && child->flags.inherit)
        {
            setBeforeHook(parent, child);
            setAfterHook(parent, child);
            setHooksAndFlags(child, &child->array);
        }
        else if (child->array.count == 0)
        {
            setBeforeHook(parent, child);
            setAfterHook(parent, child);
        }
    }
}

static void
prefix(Compiler* compiler)
{
    /* backup information */
    LineArray* parent = compiler->linePrefix;
    LineArray* dest = compiler->lineDest;

    /* advance line */
    compiler->linePrefix = compiler->lineDest;
    writeLineArray(compiler->lineDest, &compiler->line);
    compiler->lineDest = nextLineArray(compiler);
    freeLine(&compiler->line);
    initLine(&compiler->line);

    /* scan chords/prefixes */
    while (!check(compiler, TOKEN_EOF) && !check(compiler, TOKEN_RIGHT_BRACE))
    {
        keyChord(compiler);
    }
    consume(compiler, TOKEN_RIGHT_BRACE, "Expect '}' after prefix.");

    /* end prefix */
    setHooksAndFlags(getLastLine(compiler->linePrefix), compiler->lineDest);
    compiler->linePrefix = parent;
    compiler->lineDest = dest;
}

static void
synchronize(Compiler* compiler)
{
    compiler->panicMode = false;

    while (currentType(compiler) != TOKEN_EOF)
    {
        switch (currentType(compiler))
        {
        case TOKEN_LEFT_BRACKET: /* FALLTHROUGH */
        case TOKEN_LEFT_BRACE:
        case TOKEN_KEY: return;
        default: break;
        }
        advance(compiler);
    }
}

static void
keyChord(Compiler* compiler)
{
    if (match(compiler, TOKEN_LEFT_BRACKET))
    {
        chordArray(compiler); /* [abcd] 'desc' ${{echo %(key)}} */
    }
    else if (match(compiler, TOKEN_LEFT_BRACE))
    {
        prefix(compiler);
    }
    else
    {
        chord(compiler);
    }
    if (compiler->panicMode) synchronize(compiler);
}

bool
transpileChords(Compiler* compiler, const char* delimiter, bool debugFlag)
{
    assert(compiler);

    debug = debugFlag;
    advance(compiler);
    while (!match(compiler, TOKEN_EOF))
    {
        keyChord(compiler);
    }

    if (debug) debugLineArray(&compiler->lines);

    return !compiler->hadError;
}
