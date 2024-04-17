#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* common includes */
#include "common/common.h"
#include "common/debug.h"
#include "common/key_chord.h"
#include "common/memory.h"
#include "common/menu.h"
#include "common/string.h"

/* local includes */
#include "compiler.h"
#include "debug.h"
#include "scanner.h"
#include "token.h"

static void compileKeyChord(Compiler* compiler);

static bool debug = false;
static const char* delimiter = NULL;
static size_t delimiterLen = 0;
static PseudoChord nullPseudoChord = {0};

static void
initPseudoChordArray(PseudoChordArray* array)
{
    assert(array);

    array->chords = NULL;
    array->count = 0;
    array->capacity = 0;
}

static void
initPseudoChord(PseudoChord* chord)
{
    assert(chord);

    chord->state = KEY_CHORD_STATE_NOT_NULL;
    initKey(&chord->key);
    initTokenArray(&chord->description);
    initTokenArray(&chord->command);
    initTokenArray(&chord->command);
    initTokenArray(&chord->command);
    initChordFlags(&chord->flags);
    initPseudoChordArray(&chord->chords);
}

static void
makeNullPseudoChord(PseudoChord* chord)
{
    assert(chord);

    initPseudoChord(chord);
    chord->state = KEY_CHORD_STATE_IS_NULL;
}

static void
copyPseudoChord(PseudoChord* from, PseudoChord* to)
{
    assert(from && to);

    to->state = from->state;
    copyKey(&from->key, &to->key);
    copyTokenArray(&from->description, &to->description);
    copyTokenArray(&from->command, &to->command);
    copyTokenArray(&from->before, &to->before);
    copyTokenArray(&from->after, &to->after);
    copyChordFlags(&from->flags, &to->flags);
    to->chords = from->chords;
}

static void freePseudoChordArray(PseudoChordArray* chords);

static void
freePseudoChord(PseudoChord* chord)
{
    assert(chord);

    freeTokenArray(&chord->description);
    freeTokenArray(&chord->command);
    freeTokenArray(&chord->before);
    freeTokenArray(&chord->after);
    if (chord->chords.count) freePseudoChordArray(&chord->chords);
    initPseudoChord(chord);
}

void
freePseudoChordArray(PseudoChordArray* chords)
{
    assert(chords);

    for (size_t i = 0; i < chords->count; i++)
    {
        /* disassemblePseudoChord(&chords->chords[i]); */
        freePseudoChord(&chords->chords[i]);
    }
    FREE_ARRAY(PseudoChordArray, chords->chords, chords->count);
    chords->chords = NULL;
    chords->count = 0;
    chords->capacity = 0;
}

static PseudoChord*
writePseudoChordArray(PseudoChordArray* array, PseudoChord* chord)
{
    assert(array && chord);

    if (array->count == array->capacity)
    {
        size_t oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->chords = GROW_ARRAY(PseudoChord, array->chords, oldCapacity, array->capacity);
    }

    copyPseudoChord(chord, &array->chords[array->count]);
    PseudoChord* result = &array->chords[array->count++];
    if (debug) disassemblePseudoChord(result);
    return result;
}

static void
errorAtCurrent(Compiler* compiler, const char* fmt, ...)
{
    if (compiler->panicMode) return;
    compiler->panicMode = true;
    compiler->hadError = true;

    va_list ap;
    va_start(ap, fmt);
    errorAtToken(&compiler->current, compiler->scanner.filepath, fmt, ap);
    va_end(ap);
}

static TokenType
advanceCompiler(Compiler* compiler)
{
    copyToken(&compiler->current, &compiler->previous);

    while (true)
    {
        scanTokenForCompiler(&compiler->scanner, &compiler->current);
        if (debug) disassembleToken(&compiler->current);
        if (compiler->current.type != TOKEN_ERROR) break;

        errorAtCurrent(compiler, compiler->current.message);
    }

    return compiler->current.type;
}

static void
consume(Compiler* compiler, TokenType type, const char* message)
{
    if (compiler->current.type == type)
    {
        advanceCompiler(compiler);
        return;
    }

    errorAtCurrent(compiler, message);
}

static bool
checkCompiler(Compiler* compiler, TokenType type)
{
    return compiler->current.type == type;
}

static bool
compilerIsAtEnd(Compiler* compiler)
{
    return isAtEnd(&compiler->scanner);
}

static bool
matchCompiler(Compiler* compiler, TokenType type)
{
    if (!checkCompiler(compiler, type)) return false;
    advanceCompiler(compiler);
    return true;
}

static TokenType
currentType(Compiler* compiler)
{
    return compiler->current.type;
}

static Token*
currentToken(Compiler* compiler)
{
    return &compiler->current;
}

static void
synchronize(Compiler* compiler)
{
    assert(compiler);

    compiler->panicMode = false;

    while (!compilerIsAtEnd(compiler))
    {
        switch (currentType(compiler))
        {
        case TOKEN_LEFT_BRACKET: return; /* No need to skip past. */
        case TOKEN_COMMAND:
        {
            advanceCompiler(compiler);
            return;
        }
        case TOKEN_LEFT_BRACE:
        {
            while (!compilerIsAtEnd(compiler) &&
                   !checkCompiler(compiler, TOKEN_RIGHT_BRACE))
            {
                advanceCompiler(compiler);
            }
            advanceCompiler(compiler);
            return;
        }
        default: advanceCompiler(compiler); break;
        }
    }
}

static void
addMod(TokenType type, Modifiers* mod)
{
    assert(mod);

    switch (type)
    {
    case TOKEN_MOD_CTRL: mod->ctrl = true; break;
    case TOKEN_MOD_ALT: mod->alt = true; break;
    case TOKEN_MOD_HYPER: mod->hyper = true; break;
    case TOKEN_MOD_SHIFT: mod->shift = true; break;
    default: break;
    }
}

static void
compileMods(Compiler* compiler, Modifiers* mods)
{
    assert(compiler && mods);
    if (compiler->panicMode) return;

    TokenType type = currentType(compiler);
    while (isTokenModType(type))
    {
        addMod(type, mods);
        type = advanceCompiler(compiler);
    }
}

static bool
isKey(Compiler* compiler)
{
    assert(compiler);
    if (compiler->panicMode) return false;

    switch (currentType(compiler))
    {
    /* is a key */
    case TOKEN_KEY: /* FALLTHROUGH */
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
compileKeyFromToken(Compiler* compiler, PseudoChord* chord)
{
    assert(compiler && chord);
    if (compiler->panicMode) return;

    Token* token = currentToken(compiler);
    String result = {0};
    initString(&result);
    appendToString(&result, token->start, token->length);
    chord->key.repr = result.string;
    chord->key.len = result.count;
    disownString(&result);
}

static void
compileKey(Compiler* compiler, PseudoChord* chord)
{
    assert(compiler && chord);
    if (compiler->panicMode) return;

    if (!isKey(compiler)) return errorAtCurrent(compiler, "Expected key or special.");
    compileKeyFromToken(compiler, chord);
    consume(compiler, currentType(compiler), "Expected key or special.");
}

static size_t
compilerGetIndex(Compiler* compiler)
{
    return compiler->chordsDest->count;
}

/* static void */
/* compileDescription(Compiler* compiler, KeyChord* keyChord) */
/* { */
/*     assert(compiler && keyChord); */
/*     if (compiler->panicMode) return; */

/*     if (!checkCompiler(compiler, TOKEN_DESC_INTERP) && */
/*         !checkCompiler(compiler, TOKEN_DESCRIPTION)) */
/*     { */
/*         errorAtCurrent(compiler, "Expected description."); */
/*         return; */
/*     } */

/*     String result = {0}; */
/*     initString(&result); */
/*     while (!checkCompiler(compiler, TOKEN_EOF)) */
/*     { */
/*         Token* token = currentToken(compiler); */
/*         switch (token->type) */
/*         { */
/*         case TOKEN_THIS_KEY: appendToString(&result, keyChord->key.repr, keyChord->key.len); break; */
/*         case TOKEN_INDEX: appendUInt32ToString(&result, compilerGetIndex(compiler)); break; */
/*         case TOKEN_INDEX_ONE: appendUInt32ToString(&result, compilerGetIndex(compiler) + 1); break; */
/*         case TOKEN_DESC_INTERP: /\* FALLTHROUGH *\/ */
/*         case TOKEN_DESCRIPTION: appendToString(&result, token->start, token->length); break; */
/*         default: errorAtCurrent(compiler, "Malfromed description."); goto fail; */
/*         } */
/*         if (checkCompiler(compiler, TOKEN_DESCRIPTION)) break; */
/*         advanceCompiler(compiler); */
/*     } */

/*     rtrimString(&result); */
/*     keyChord->description = result.string; */
/*     disownString(&result); */
/*     consume(compiler, TOKEN_DESCRIPTION, "Expect description."); */
/*     return; */

/* fail: */
/*     freeString(&result); */
/*     return; */
/* } */

/* static void */
/* compileCommand(Compiler* compiler, KeyChord* keyChord, char** dest) */
/* { */
/*     assert(compiler && keyChord && dest); */
/*     if (compiler->panicMode) return; */

/*     /\* No longer in a chord expression but not an error *\/ */
/*     if (checkCompiler(compiler, TOKEN_RIGHT_PAREN)) return; */

/*     if (!checkCompiler(compiler, TOKEN_COMM_INTERP) && */
/*         !checkCompiler(compiler, TOKEN_COMMAND)) */
/*     { */
/*         errorAtCurrent(compiler, "Expected command."); */
/*         return; */
/*     } */

/*     String result = {0}; */
/*     initString(&result); */
/*     while (!compilerIsAtEnd(compiler)) */
/*     { */
/*         Token* token = currentToken(compiler); */
/*         switch (token->type) */
/*         { */
/*         case TOKEN_THIS_KEY: appendToString(&result, keyChord->key.repr, keyChord->key.len); break; */
/*         case TOKEN_THIS_DESC: */
/*         { */
/*             appendToString(&result, keyChord->description, strlen(keyChord->description)); */
/*             break; */
/*         } */
/*         case TOKEN_THIS_DESC_UPPER_FIRST: */
/*         { */
/*             appendToStringWithState( */
/*                 &result, keyChord->description, */
/*                 strlen(keyChord->description), STRING_APPEND_UPPER_FIRST */
/*             ); */
/*             break; */
/*         } */
/*         case TOKEN_THIS_DESC_LOWER_FIRST: */
/*         { */
/*             appendToStringWithState( */
/*                 &result, keyChord->description, */
/*                 strlen(keyChord->description), STRING_APPEND_LOWER_FIRST */
/*             ); */
/*             break; */
/*         } */
/*         case TOKEN_THIS_DESC_UPPER_ALL: */
/*         { */
/*             appendToStringWithState( */
/*                 &result, keyChord->description, */
/*                 strlen(keyChord->description), STRING_APPEND_UPPER_ALL */
/*             ); */
/*             break; */
/*         } */
/*         case TOKEN_THIS_DESC_LOWER_ALL: */
/*         { */
/*             appendToStringWithState( */
/*                 &result, keyChord->description, */
/*                 strlen(keyChord->description), STRING_APPEND_LOWER_ALL */
/*             ); */
/*             break; */
/*         } */
/*         case TOKEN_INDEX: appendUInt32ToString(&result, compilerGetIndex(compiler)); break; */
/*         case TOKEN_INDEX_ONE: appendUInt32ToString(&result, compilerGetIndex(compiler) + 1); break; */
/*         case TOKEN_COMM_INTERP: /\* FALLTHROUGH *\/ */
/*         case TOKEN_COMMAND: appendToString(&result, token->start, token->length); break; */
/*         default: errorAtCurrent(compiler, "Malformed command."); goto fail; */
/*         } */
/*         if (checkCompiler(compiler, TOKEN_COMMAND)) break; */
/*         advanceCompiler(compiler); */
/*     } */

/*     rtrimString(&result); */
/*     *dest = result.string; */
/*     disownString(&result); */
/*     consume(compiler, TOKEN_COMMAND, "Expected command."); */
/*     return; */

/* fail: */
/*     freeString(&result); */
/*     return; */
/* } */

static void
collectDescriptionTokens(Compiler* compiler, TokenArray* tokens)
{
    assert(compiler && tokens);
    if (compiler->panicMode) return;

    if (!checkCompiler(compiler, TOKEN_DESC_INTERP) &&
        !checkCompiler(compiler, TOKEN_DESCRIPTION))
    {
        errorAtCurrent(compiler, "Expected description.");
        return;
    }

    while (!checkCompiler(compiler, TOKEN_EOF))
    {
        Token* token = currentToken(compiler);
        switch (token->type)
        {
        case TOKEN_THIS_KEY: /* FALLTHROUGH */
        case TOKEN_INDEX:
        case TOKEN_INDEX_ONE:
        case TOKEN_DESC_INTERP:
        case TOKEN_DESCRIPTION: writeTokenArray(tokens, token); break;
        default: errorAtCurrent(compiler, "Malfromed description."); return;
        }
        if (checkCompiler(compiler, TOKEN_DESCRIPTION)) break;
        advanceCompiler(compiler);
    }

    consume(compiler, TOKEN_DESCRIPTION, "Expect description.");
    return;
}

static void
collectCommandTokens(Compiler* compiler, TokenArray* tokens)
{
    assert(compiler && tokens);
    if (compiler->panicMode) return;

    if (!checkCompiler(compiler, TOKEN_COMM_INTERP) &&
        !checkCompiler(compiler, TOKEN_COMMAND))
    {
        errorAtCurrent(compiler, "Expected command.");
        return;
    }

    while (!checkCompiler(compiler, TOKEN_EOF))
    {
        Token* token = currentToken(compiler);
        switch (token->type)
        {
        case TOKEN_THIS_KEY: /* FALLTHROUGH */
        case TOKEN_INDEX:
        case TOKEN_INDEX_ONE:
        case TOKEN_THIS_DESC:
        case TOKEN_THIS_DESC_UPPER_FIRST:
        case TOKEN_THIS_DESC_LOWER_FIRST:
        case TOKEN_THIS_DESC_UPPER_ALL:
        case TOKEN_THIS_DESC_LOWER_ALL:
        case TOKEN_COMM_INTERP:
        case TOKEN_COMMAND: writeTokenArray(tokens, token); break;
        default: errorAtCurrent(compiler, "Malfromed command."); return;
        }
        if (checkCompiler(compiler, TOKEN_COMMAND)) break;
        advanceCompiler(compiler);
    }

    consume(compiler, TOKEN_COMMAND, "Expect command.");
    return;
}

static void
compileHint(Compiler* compiler, KeyChord* keyChord)
{
    assert(compiler && keyChord);
    if (compiler->panicMode) return;

    String result = {0};
    initString(&result);
    Modifiers* mods = &keyChord->key.mods;
    if (mods->ctrl) appendToString(&result, "C-", strlen("C-"));
    if (mods->alt) appendToString(&result, "A-", strlen("A-"));
    if (mods->hyper) appendToString(&result, "H-", strlen("H-"));
    if (mods->shift) appendToString(&result, "S-", strlen("S-"));
    appendToString(&result, keyChord->key.repr, keyChord->key.len);
    appendToString(&result, delimiter, delimiterLen);
    appendToString(&result, keyChord->description, strlen(keyChord->description));
    keyChord->hint = result.string;
    disownString(&result);
}

static bool
compileHook(Compiler* compiler, PseudoChord* chord, TokenType type)
{
    assert(compiler && chord);
    if (compiler->panicMode) return false;

    switch (type)
    {
    case TOKEN_BEFORE:
    {
        consume(compiler, TOKEN_BEFORE, "Expected '^before' hook.");
        if (!checkCompiler(compiler, TOKEN_COMMAND) && !checkCompiler(compiler, TOKEN_COMM_INTERP))
        {
            errorAtCurrent(compiler, "Expected command after '^before' hook.");
            return false;
        }
        collectCommandTokens(compiler, &chord->before);
        return true;
    }
    case TOKEN_AFTER:
    {
        consume(compiler, TOKEN_BEFORE, "Expected '^after' hook.");
        if (!checkCompiler(compiler, TOKEN_COMMAND) && !checkCompiler(compiler, TOKEN_COMM_INTERP))
        {
            errorAtCurrent(compiler, "Expected command after '^after' hook.");
            return false;
        }
        collectCommandTokens(compiler, &chord->after);
        return true;
    }
    case TOKEN_SYNC_BEFORE:
    {
        consume(compiler, TOKEN_BEFORE, "Expected '^sync-before' hook.");
        if (!checkCompiler(compiler, TOKEN_COMMAND) && !checkCompiler(compiler, TOKEN_COMM_INTERP))
        {
            errorAtCurrent(compiler, "Expected command after '^sync-before' hook.");
            return false;
        }
        collectCommandTokens(compiler, &chord->before);
        chord->flags.syncBefore = true;
        return true;
    }
    case TOKEN_SYNC_AFTER:
    {
        consume(compiler, TOKEN_BEFORE, "Expected '^sync-after' hook.");
        if (!checkCompiler(compiler, TOKEN_COMMAND) && !checkCompiler(compiler, TOKEN_COMM_INTERP))
        {
            errorAtCurrent(compiler, "Expected command after '^sync-after' hook.");
            return false;
        }
        collectCommandTokens(compiler, &chord->after);
        chord->flags.syncAfter = true;
        return true;
    }
    default: return false;
    }
}

static bool
compileFlag(Compiler* compiler, PseudoChord* chord, TokenType type)
{
    assert(compiler && chord);
    if (compiler->panicMode) return false;

    switch (type)
    {
    case TOKEN_KEEP: chord->flags.keep = true; return true;
    case TOKEN_CLOSE: chord->flags.close = true; return true;
    case TOKEN_INHERIT: chord->flags.inherit = true; return true;
    case TOKEN_IGNORE: chord->flags.ignore = true; return true;
    case TOKEN_UNHOOK: chord->flags.unhook = true; return true;
    case TOKEN_DEFLAG: chord->flags.deflag = true; return true;
    case TOKEN_NO_BEFORE: chord->flags.nobefore = true; return true;
    case TOKEN_NO_AFTER: chord->flags.noafter = true; return true;
    case TOKEN_WRITE: chord->flags.write = true; return true;
    case TOKEN_SYNC_CMD: chord->flags.syncCommand = true; return true;
    default: return false;
    }
}

static void
compileHooksAndFlags(Compiler* compiler, PseudoChord* keyChord)
{
    assert(compiler && keyChord);
    if (compiler->panicMode) return;

    TokenType type = currentType(compiler);
    while (!compilerIsAtEnd(compiler))
    {
        if (!compileHook(compiler, keyChord, type) && !compileFlag(compiler, keyChord, type)) return;

        /* Hooks consume their own token and their command
         * which leaves them pointing at the next token. */
        type = isTokenHookType(type) ? currentType(compiler) : advanceCompiler(compiler);
    }
}

static void
compileChord(Compiler* compiler)
{
    assert(compiler);

    PseudoChord* chord = &compiler->chord;
    initPseudoChord(chord);

    compileMods(compiler, &chord->key.mods);
    compileKey(compiler, chord);
    collectDescriptionTokens(compiler, &chord->description);
    /* Don't compile hint until the end of scope */
    compileHooksAndFlags(compiler, chord);

    /* Prefix */
    if (checkCompiler(compiler, TOKEN_LEFT_BRACE)) return;

    collectCommandTokens(compiler, &chord->command);

    /* Check for brace after command */
    if (checkCompiler(compiler, TOKEN_LEFT_BRACE))
    {
        errorAtCurrent(compiler, "Expected end of key chord after command but got '{'.");
        return;
    }

    writePseudoChordArray(compiler->chordsDest, chord);
    freePseudoChord(chord);
}

static void
compileStringFromTokens(TokenArray* tokens, KeyChord* to, char** dest, size_t index)
{
    assert(tokens && to && dest);

    String result = {0};
    initString(&result);

    for (size_t i = 0; i < tokens->count; i++)
    {
        Token* token = &tokens->tokens[i];
        switch (token->type)
        {
        case TOKEN_THIS_KEY: appendToString(&result, to->key.repr, to->key.len); break;
        case TOKEN_THIS_DESC:
        {
            appendToString(&result, to->description, strlen(to->description));
            break;
        }
        case TOKEN_THIS_DESC_UPPER_FIRST:
        {
            appendToStringWithState(
                &result, to->description,
                strlen(to->description), STRING_APPEND_UPPER_FIRST
            );
            break;
        }
        case TOKEN_THIS_DESC_LOWER_FIRST:
        {
            appendToStringWithState(
                &result, to->description,
                strlen(to->description), STRING_APPEND_LOWER_FIRST
            );
            break;
        }
        case TOKEN_THIS_DESC_UPPER_ALL:
        {
            appendToStringWithState(
                &result, to->description,
                strlen(to->description), STRING_APPEND_UPPER_ALL
            );
            break;
        }
        case TOKEN_THIS_DESC_LOWER_ALL:
        {
            appendToStringWithState(
                &result, to->description,
                strlen(to->description), STRING_APPEND_LOWER_ALL
            );
            break;
        }
        case TOKEN_INDEX: appendUInt32ToString(&result, index); break;
        case TOKEN_INDEX_ONE: appendUInt32ToString(&result, index + 1); break;
        case TOKEN_DESC_INTERP: /* FALLTHROUGH */
        case TOKEN_DESCRIPTION:
        case TOKEN_COMM_INTERP:
        case TOKEN_COMMAND: appendToString(&result, token->start, token->length); break;
        default:
        {
            errorMsg("Got strang token in chord array: '%s'.", getTokenRepr(token->type));
            break;
        }
        }
    }

    rtrimString(&result);
    *dest = result.string;
    disownString(&result);
}

/* static void */
/* compileString(char* from, char** to) */
/* { */
/*     assert(from && to); */

/*     String result = {0}; */
/*     initString(&result); */
/*     appendToString(&result, from, strlen(from)); */
/*     *to = result.string; */
/*     disownString(&result); */
/* } */

static void
compileMissingKeyChordInfo(
    Compiler* compiler,
    PseudoChord* from,
    PseudoChord* to,
    TokenArray* descriptionTokens,
    TokenArray* commandTokens,
    size_t index)
{
    assert(compiler && from && to && descriptionTokens && commandTokens);

    if (!to->description.count) copyTokenArray(&from->description, &to->description);
    if (!to->command.count) copyTokenArray(&from->command, &to->command);
    if (!to->before.count && from->before.count) copyTokenArray(&from->before, &to->before);
    if (!to->after.count && from->after.count) copyTokenArray(&from->after, &to->after);
    if (hasDefaultChordFlags(&to->flags)) copyChordFlags(&from->flags, &to->flags);
}

static void
compileChordArray(Compiler* compiler)
{
    assert(compiler);

    if (!isKey(compiler) &&
        !isTokenModType(currentType(compiler)) &&
        !checkCompiler(compiler, TOKEN_LEFT_PAREN))
    {
        errorAtCurrent(compiler, "Expect modifier, key, or chord expression after '['.");
        return;
    }

    size_t arrayStart = compilerGetIndex(compiler);

    while (!compilerIsAtEnd(compiler) &&
           !checkCompiler(compiler, TOKEN_RIGHT_BRACKET))
    {
        if (!isKey(compiler) &&
            !isTokenModType(currentType(compiler)) &&
            !checkCompiler(compiler, TOKEN_LEFT_PAREN))
        {
            errorAtCurrent(
                compiler,
                "Chord arrays may only contain modifiers, keys, and chord expressions."
            );
            goto fail;
        }

        PseudoChord* chord = &compiler->chord;
        initPseudoChord(chord);

        if (matchCompiler(compiler, TOKEN_LEFT_PAREN))
        {
            compileMods(compiler, &chord->key.mods);
            compileKey(compiler, chord);
            collectDescriptionTokens(compiler, &chord->description);
            /* don't compile command until the end */
            compileHooksAndFlags(compiler, chord);
            collectCommandTokens(compiler, &chord->command);
            consume(compiler, TOKEN_RIGHT_PAREN, "Expect closing parenthesis after '('.");
        }
        else
        {
            compileMods(compiler, &chord->key.mods);
            compileKey(compiler, chord);
        }

        writePseudoChordArray(compiler->chordsDest, chord);
        freePseudoChord(chord);
    }

    consume(compiler, TOKEN_RIGHT_BRACKET, "Expect ']' after key list.");
    PseudoChord dummy = {0};
    initPseudoChord(&dummy);

    collectDescriptionTokens(compiler, &dummy.description);
    compileHooksAndFlags(compiler, &dummy);
    collectCommandTokens(compiler, &dummy.command);

    /* Write chords in chord array to destination */
    PseudoChordArray* array = compiler->chordsDest;
    size_t arrayEnd = array->count;
    for (size_t i = arrayStart; i < arrayEnd; i++)
    {
        PseudoChord* chord = &array->chords[i];
        compileMissingKeyChordInfo(
            compiler, &dummy, chord, &dummy.description, &dummy.command, i
        );
    }

    freePseudoChord(&dummy);
    return;

fail:
    compiler->hadError = true;
    freePseudoChord(&compiler->chord);
    return;
}

static void
compileNullKeyChord(Compiler* compiler)
{
    assert(compiler);

    writePseudoChordArray(compiler->chordsDest, &nullPseudoChord);
}

static void
setBeforeHook(PseudoChord* parent, PseudoChord* child)
{
    assert(parent && child);

    /* Children that opt out do not inherit */
    if (child->flags.nobefore || !parent->before.count) return;

    copyTokenArray(&parent->before, &child->before);

    /* set syncBefore flag */
    if (parent->flags.syncBefore) child->flags.syncBefore = parent->flags.syncBefore;
}

static void
setAfterHook(PseudoChord* parent, PseudoChord* child)
{
    assert(parent && child);

    /* Children that opt out do not inherit */
    if (child->flags.noafter || !parent->after.count) return;

    copyTokenArray(&parent->after, &child->after);

    /* set syncAfter flag */
    if (parent->flags.syncAfter) child->flags.syncAfter = parent->flags.syncAfter;
}

static void
setHooks(PseudoChord* parent, PseudoChord* child)
{
    assert(parent && child);

    /* Children that opt out do not inherit */
    if (child->flags.unhook) return;

    setBeforeHook(parent, child);
    setAfterHook(parent, child);
}

static void
setFlags(ChordFlags* parent, ChordFlags* child)
{
    assert(parent && child);

    /* Children that opt out do not inherit */
    if (child->deflag) return;

    if (!child->close && parent->keep) child->keep = parent->keep;
    if (!child->execute && parent->write) child->write = parent->write;
    if (parent->syncCommand) child->syncCommand = parent->syncCommand;
}

static void
setHooksAndFlags(PseudoChord* parent, PseudoChordArray* children)
{
    assert(parent && children);

    for (size_t i = 0; i < children->count; i++)
    {
        PseudoChord* child = &children->chords[i];
        /* Don't make any changes to defiant children */
        if (child->flags.ignore) continue;

        /* Prefixes don't inherit unless requested */
        if (child->chords.count && !child->flags.inherit) continue;

        setHooks(parent, child);
        setFlags(&parent->flags, &child->flags);
        if (child->chords.count)
        {
            setHooksAndFlags(child, &child->chords);
        }
    }
}

static int
compareKeyChords(const void* a, const void* b)
{
    const Key* keyA = &((PseudoChord*)a)->key;
    const Key* keyB = &((PseudoChord*)b)->key;

    bool hasModsA = hasActiveModifier(&keyA->mods);
    bool hasModsB = hasActiveModifier(&keyB->mods);

    if (hasModsA == hasModsB) return strcmp(keyA->repr, keyB->repr);
    if (hasModsA) return 1;
    return -1;
}

static void
sortPseudoChordArray(PseudoChord* chords, size_t count)
{
    qsort(chords, count, sizeof(PseudoChord), compareKeyChords);
}

static void
compilePrefix(Compiler* compiler)
{
    assert(compiler);

    /* Backup information */
    PseudoChordArray* previousDest = compiler->chordsDest;

    /* advance */
    PseudoChord* parent = writePseudoChordArray(compiler->chordsDest, &compiler->chord);
    PseudoChordArray* children = &parent->chords;
    compiler->chordsDest = children;

    /* start new scope */
    initPseudoChordArray(compiler->chordsDest);
    freePseudoChord(&compiler->chord);

    /* Compile children */
    while (!compilerIsAtEnd(compiler) && !checkCompiler(compiler, TOKEN_RIGHT_BRACE))
    {
        compileKeyChord(compiler);
    }
    consume(compiler, TOKEN_RIGHT_BRACE, "Expect '}' after prefix.");


    /* end prefix */
    setHooksAndFlags(parent, compiler->chordsDest);
    if (compiler->sort) sortPseudoChordArray(compiler->chordsDest->chords, compilerGetIndex(compiler));
    compileNullKeyChord(compiler);
    compiler->chordsDest = previousDest;
}

static void
compileKeyChord(Compiler* compiler)
{
    assert(compiler);

    if (compiler->panicMode) synchronize(compiler);

    if (matchCompiler(compiler, TOKEN_LEFT_BRACKET))
    {
        compileChordArray(compiler);
    }
    else if (matchCompiler(compiler, TOKEN_LEFT_BRACE))
    {
        compilePrefix(compiler);
    }
    else
    {
        compileChord(compiler);
    }
}

static KeyChord*
compileFromPseudoChords(Compiler* compiler, KeyChord** dest)
{
    assert(compiler && dest);

    PseudoChordArray* array = compiler->chordsDest;
    size_t count = compiler->chordsDest->count;
    *dest = ALLOCATE(KeyChord, count);
    for (size_t i = 0; i < count; i++)
    {
        KeyChord* chord = &(*dest)[i];
        PseudoChord* pseudo = &array->chords[i];
        initKeyChord(chord);
        /* Reached null key chord */
        if (pseudo->state == KEY_CHORD_STATE_IS_NULL)
        {
            makeNullKeyChord(chord);
            break;
        }
        /* Set state */
        chord->state = KEY_CHORD_STATE_NOT_NULL;
        /* Key */
        copyKey(&pseudo->key, &chord->key);
        /* Description */
        compileStringFromTokens(&pseudo->description, chord, &chord->description, i);
        /* Hint */
        compileHint(compiler, chord);
        /* Hooks */
        compileStringFromTokens(&pseudo->before, chord, &chord->before, i);
        compileStringFromTokens(&pseudo->after, chord, &chord->after, i);
        copyChordFlags(&pseudo->flags, &chord->flags);
        /* Command */
        compileStringFromTokens(&pseudo->command, chord, &chord->command, i);
        if (pseudo->chords.count)
        {
            PseudoChordArray* parent = compiler->chordsDest;
            PseudoChordArray* children = &pseudo->chords;
            compiler->chordsDest = children;
            compileFromPseudoChords(compiler, &chord->keyChords);
            compiler->chordsDest = parent;
        }
    }

    freePseudoChordArray(compiler->chordsDest);
    return *dest;
}

KeyChord*
compileKeyChords(Compiler* compiler, Menu* menu)
{
    assert(compiler && menu);

    /* Initialize globals and defaults */
    makeNullPseudoChord(&nullPseudoChord);
    debug = menu->debug;
    delimiter = menu->delimiter;
    delimiterLen = strlen(delimiter);
    compiler->sort = menu->sort;

    if (debug) debugPrintScannedTokenHeader();

    advanceCompiler(compiler);
    while (!compilerIsAtEnd(compiler))
    {
        compileKeyChord(compiler);
    }

    if (compiler->sort) sortPseudoChordArray(compiler->chordsDest->chords, compilerGetIndex(compiler));
    compileNullKeyChord(compiler);

    menu->keyChords = compileFromPseudoChords(compiler, &menu->keyChordsHead);

    if (debug)
    {
        debugPrintScannedTokenFooter();
        disassembleKeyChords(menu->keyChordsHead, 0);
    }

    return compiler->hadError ? NULL : menu->keyChords;
}

void
initCompiler(Compiler* compiler, char *source, const char *filepath)
{
    assert(compiler && source && filepath);

    initScanner(&compiler->scanner, source, filepath);
    compiler->hadError = false;
    compiler->panicMode = false;
    initPseudoChordArray(&compiler->chords);
    compiler->chordsDest = &compiler->chords;
    compiler->source = source;
    compiler->sort = false;
}
