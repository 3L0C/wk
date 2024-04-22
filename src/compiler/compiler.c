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
    initToken(&chord->keyToken);
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
    assert(from), assert(to);

    to->state = from->state;
    copyKey(&from->key, &to->key);
    copyToken(&from->keyToken, &to->keyToken);
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

    if (chord->key.repr) free(chord->key.repr);
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
        freePseudoChord(&chords->chords[i]);
    }
    FREE_ARRAY(PseudoChordArray, chords->chords, chords->count);
    chords->chords = NULL;
    chords->count = 0;
    chords->capacity = 0;
}

static PseudoChord*
writePseudoChordArray(Compiler* compiler, PseudoChord* chord)
{
    assert(compiler), assert(chord);

    PseudoChordArray* array = compiler->chordsDest;
    if (array->count == array->capacity)
    {
        size_t oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->chords = GROW_ARRAY(PseudoChord, array->chords, oldCapacity, array->capacity);
    }

    copyPseudoChord(chord, &array->chords[array->count]);
    PseudoChord* result = &array->chords[array->count++];
    return result;
}

static void
errorAtCurrent(Compiler* compiler, const char* fmt, ...)
{
    assert(compiler), assert(fmt);
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
    assert(compiler);

    copyToken(&compiler->current, &compiler->previous);

    while (true)
    {
        scanTokenForCompiler(&compiler->scanner, &compiler->current);
        if (compiler->debug) disassembleToken(&compiler->current);
        if (compiler->current.type != TOKEN_ERROR) break;

        errorAtCurrent(compiler, compiler->current.message);
    }

    return compiler->current.type;
}

static bool
consume(Compiler* compiler, TokenType type, const char* message)
{
    assert(compiler), assert(message);

    if (compiler->current.type == type)
    {
        advanceCompiler(compiler);
        return true;
    }

    errorAtCurrent(compiler, message);
    return false;
}

static bool
checkCompiler(Compiler* compiler, TokenType type)
{
    assert(compiler);

    return compiler->current.type == type;
}

static bool
compilerIsAtEnd(Compiler* compiler)
{
    assert(compiler);

    return isAtEnd(&compiler->scanner);
}

static bool
matchCompiler(Compiler* compiler, TokenType type)
{
    assert(compiler);

    if (!compilerIsAtEnd(compiler) && !checkCompiler(compiler, type)) return false;
    advanceCompiler(compiler);
    return true;
}

static TokenType
currentType(Compiler* compiler)
{
    assert(compiler);

    return compiler->current.type;
}

static Token*
currentToken(Compiler* compiler)
{
    assert(compiler);

    return &compiler->current;
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
    assert(compiler), assert(mods);
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
    case TOKEN_KEY: /* FALLTHROUGH */
    case TOKEN_SPECIAL_KEY: return true;
    default: return false;
    }

    return false;
}

static SpecialKey
compilerGetSpecial(Compiler* compiler)
{
    assert(compiler);

    return compiler->scanner.special;
}

static void
compileKey(Compiler* compiler, PseudoChord* chord)
{
    assert(compiler), assert(chord);
    if (compiler->panicMode) return;

    if (!isKey(compiler)) return errorAtCurrent(compiler, "Expected key or special.");
    Token* token = currentToken(compiler);
    if (token->type == TOKEN_SPECIAL_KEY) chord->key.special = compilerGetSpecial(compiler);
    copyToken(token, &chord->keyToken);
    consume(compiler, currentType(compiler), "Expected key or special.");
}

static size_t
compilerGetIndex(Compiler* compiler)
{
    assert(compiler);

    return compiler->chordsDest->count;
}

static void
collectDescriptionTokens(Compiler* compiler, TokenArray* tokens)
{
    assert(compiler), assert(tokens);
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

static bool
collectCommandTokens(Compiler* compiler, TokenArray* tokens, bool inChordArray, const char* message)
{
    assert(compiler), assert(tokens), assert(message);
    if (compiler->panicMode) return false;
    if (inChordArray && checkCompiler(compiler, TOKEN_RIGHT_PAREN)) return false;

    if (!checkCompiler(compiler, TOKEN_COMM_INTERP) &&
        !checkCompiler(compiler, TOKEN_COMMAND))
    {
        errorAtCurrent(compiler, message);
        return false;
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
        default: errorAtCurrent(compiler, "Malfromed command."); return false;
        }
        if (checkCompiler(compiler, TOKEN_COMMAND)) break;
        advanceCompiler(compiler);
    }

    return consume(compiler, TOKEN_COMMAND, "Expected end of command.");
}

static bool
compileHook(Compiler* compiler, PseudoChord* chord, TokenType type)
{
    assert(compiler), assert(chord);
    if (compiler->panicMode) return false;

    switch (type)
    {
    case TOKEN_BEFORE:
    {
        consume(compiler, TOKEN_BEFORE, "Expected '^before' hook.");
        return collectCommandTokens(
            compiler, &chord->before, false, "Expected command after '^before' hook."
        );
    }
    case TOKEN_AFTER:
    {
        consume(compiler, TOKEN_BEFORE, "Expected '^after' hook.");
        return collectCommandTokens(
            compiler, &chord->after, false, "Expected command after '^after' hook."
        );
    }
    case TOKEN_SYNC_BEFORE:
    {
        consume(compiler, TOKEN_BEFORE, "Expected '^sync-before' hook.");
        chord->flags.syncBefore = true;
        return collectCommandTokens(
            compiler, &chord->before, false, "Expected command after '^sync-before' hook."
        );
    }
    case TOKEN_SYNC_AFTER:
    {
        consume(compiler, TOKEN_BEFORE, "Expected '^sync-after' hook.");
        chord->flags.syncAfter = true;
        return collectCommandTokens(
            compiler, &chord->after, false, "Expected command after '^sync-after' hook."
        );
    }
    default: return false;
    }
}

static bool
compileFlag(Compiler* compiler, PseudoChord* chord, TokenType type)
{
    assert(compiler), assert(chord);
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
    assert(compiler), assert(keyChord);
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
    freePseudoChord(chord);
    initPseudoChord(chord);

    compileMods(compiler, &chord->key.mods);
    compileKey(compiler, chord);
    collectDescriptionTokens(compiler, &chord->description);
    /* Don't compile hint until the end of scope */
    compileHooksAndFlags(compiler, chord);

    /* Prefix */
    if (checkCompiler(compiler, TOKEN_LEFT_BRACE)) return;

    collectCommandTokens(compiler, &chord->command, false, "Expected command.");

    /* Check for brace after command */
    if (checkCompiler(compiler, TOKEN_LEFT_BRACE))
    {
        errorAtCurrent(compiler, "Expected end of key chord after command but got '{'.");
        return;
    }

    writePseudoChordArray(compiler, chord);
    freePseudoChord(chord);
}

static void
compileMissingKeyChordInfo(
    Compiler* compiler,
    PseudoChord* from,
    PseudoChord* to,
    TokenArray* descriptionTokens,
    TokenArray* commandTokens,
    size_t index)
{
    assert(compiler), assert(from), assert(to), assert(descriptionTokens), assert(commandTokens);

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
            collectCommandTokens(compiler, &chord->command, true, "Expected command.");
            consume(compiler, TOKEN_RIGHT_PAREN, "Expect closing parenthesis after '('.");
        }
        else
        {
            compileMods(compiler, &chord->key.mods);
            compileKey(compiler, chord);
        }

        writePseudoChordArray(compiler, chord);
        freePseudoChord(chord);
    }

    consume(compiler, TOKEN_RIGHT_BRACKET, "Expect ']' after chord array.");
    PseudoChord dummy = {0};
    initPseudoChord(&dummy);

    collectDescriptionTokens(compiler, &dummy.description);
    compileHooksAndFlags(compiler, &dummy);
    collectCommandTokens(compiler, &dummy.command, false, "Expected command.");

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

    writePseudoChordArray(compiler, &compiler->nullPseudoChord);
}

static void
setBeforeHook(PseudoChord* parent, PseudoChord* child)
{
    assert(parent), assert(child);

    /* Children that opt out do not inherit */
    if (child->flags.nobefore || !parent->before.count) return;

    copyTokenArray(&parent->before, &child->before);

    /* set syncBefore flag */
    if (parent->flags.syncBefore) child->flags.syncBefore = parent->flags.syncBefore;
}

static void
setAfterHook(PseudoChord* parent, PseudoChord* child)
{
    assert(parent), assert(child);

    /* Children that opt out do not inherit */
    if (child->flags.noafter || !parent->after.count) return;

    copyTokenArray(&parent->after, &child->after);

    /* set syncAfter flag */
    if (parent->flags.syncAfter) child->flags.syncAfter = parent->flags.syncAfter;
}

static void
setHooks(PseudoChord* parent, PseudoChord* child)
{
    assert(parent), assert(child);

    /* Children that opt out do not inherit */
    if (child->flags.unhook) return;

    setBeforeHook(parent, child);
    setAfterHook(parent, child);
}

static void
setFlags(ChordFlags* parent, ChordFlags* child)
{
    assert(parent), assert(child);

    /* Children that opt out do not inherit */
    if (child->deflag) return;

    if (!child->close && parent->keep) child->keep = parent->keep;
    if (!child->execute && parent->write) child->write = parent->write;
    if (parent->syncCommand) child->syncCommand = parent->syncCommand;
}

static void
setHooksAndFlags(PseudoChord* parent, PseudoChordArray* children)
{
    assert(parent), assert(children);

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
    assert(a), assert(b);

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
    assert(chords);

    qsort(chords, count, sizeof(PseudoChord), compareKeyChords);
}

static void
compilePrefix(Compiler* compiler)
{
    assert(compiler);

    /* Backup information */
    PseudoChordArray* previousDest = compiler->chordsDest;

    /* advance */
    PseudoChord* parent = writePseudoChordArray(compiler, &compiler->chord);
    freePseudoChord(&compiler->chord);
    PseudoChordArray* children = &parent->chords;
    compiler->chordsDest = children;

    /* start new scope */
    initPseudoChordArray(compiler->chordsDest);

    /* Compile children */
    while (!compilerIsAtEnd(compiler) && !checkCompiler(compiler, TOKEN_RIGHT_BRACE))
    {
        compileKeyChord(compiler);
    }
    consume(compiler, TOKEN_RIGHT_BRACE, "Expect '}' after prefix.");


    /* end prefix */
    setHooksAndFlags(parent, compiler->chordsDest);
    if (compiler->sort) sortPseudoChordArray(children->chords, compilerGetIndex(compiler));
    compileNullKeyChord(compiler);
    compiler->chordsDest = previousDest;
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
        /* Return on chord array start '[' */
        case TOKEN_LEFT_BRACKET: return; /* No need to skip past. */
        /* Seek to modifier or key and return. */
        case TOKEN_COMM_INTERP: /* FALLTHROUGH */
        case TOKEN_COMMAND:
        {
            /* Will consume until TOKEN_COMMAND */
            while (!compilerIsAtEnd(compiler) && !checkCompiler(compiler, TOKEN_COMMAND))
            {
                advanceCompiler(compiler);
            }
            consume(compiler, TOKEN_COMMAND, "Expected command.");
            if (!isTokenModType(currentType(compiler)) && !isKey(compiler))
            {
                while (!compilerIsAtEnd(compiler))
                {
                    if (isTokenModType(currentType(compiler)) || isKey(compiler)) break;
                    advanceCompiler(compiler);
                }
            }
            return;
        }
        /* Return on prefix start '{' */
        case TOKEN_LEFT_BRACE: return;
        default: advanceCompiler(compiler); break;
        }
    }
}

static void
compileKeyChord(Compiler* compiler)
{
    assert(compiler);

    if (compiler->panicMode) synchronize(compiler);
    if (compilerIsAtEnd(compiler)) return;

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

static void
compileKeyFromToken(Token* token, Key* key)
{
    assert(token), assert(key);

    String result = {0};
    initString(&result);
    appendToString(&result, token->start, token->length);
    key->repr = result.string;
    key->len = result.count;
    disownString(&result);
}

static void
compileDescriptionWithState(String* result, TokenType type, const char* desc)
{
    assert(result), assert(desc);


    StringAppendState state;
    switch (type)
    {
    case TOKEN_THIS_DESC_UPPER_FIRST: state = STRING_APPEND_UPPER_FIRST; break;
    case TOKEN_THIS_DESC_LOWER_FIRST: state = STRING_APPEND_LOWER_FIRST; break;
    case TOKEN_THIS_DESC_UPPER_ALL: state = STRING_APPEND_UPPER_ALL; break;
    case TOKEN_THIS_DESC_LOWER_ALL: state = STRING_APPEND_LOWER_ALL; break;
    default:
    {
        errorMsg("Got unexpected token type to `compileDescriptionWithState`.");
        appendToString(result, desc, strlen(desc));
        return;
    }
    }

    appendToStringWithState(result, desc, strlen(desc), state);
}

static void
compileStringFromToken(Token* token, KeyChord* to, String* result, size_t index)
{
    assert(token), assert(to), assert(result);


    switch (token->type)
    {
    case TOKEN_THIS_KEY: appendToString(result, to->key.repr, to->key.len); break;
    case TOKEN_THIS_DESC: appendToString(result, to->description, strlen(to->description)); break;
    case TOKEN_THIS_DESC_UPPER_FIRST: /* FALLTHROUGH */
    case TOKEN_THIS_DESC_LOWER_FIRST:
    case TOKEN_THIS_DESC_UPPER_ALL:
    case TOKEN_THIS_DESC_LOWER_ALL:
    {
        compileDescriptionWithState(result, token->type, to->description);
        break;
    }
    case TOKEN_INDEX: appendUInt32ToString(result, index); break;
    case TOKEN_INDEX_ONE: appendUInt32ToString(result, index + 1); break;
    case TOKEN_DESC_INTERP: /* FALLTHROUGH */
    case TOKEN_DESCRIPTION: appendEscStringToString(result, token->start, token->length); break;
    case TOKEN_COMM_INTERP: /* FALLTHROUGH */
    case TOKEN_COMMAND: appendToString(result, token->start, token->length); break;
    default:
    {
        errorMsg(
            "Got unexpected token when compiling token array: '%s'.",
            getTokenLiteral(token->type)
        );
        break;
    }
    }
}

static void
compileStringFromTokens(TokenArray* tokens, KeyChord* to, char** dest, size_t index)
{
    assert(tokens), assert(to), assert(dest);

    String result = {0};
    initString(&result);

    for (size_t i = 0; i < tokens->count; i++)
    {
        compileStringFromToken(&tokens->tokens[i], to, &result, index);
    }

    rtrimString(&result);
    *dest = result.string;
    disownString(&result);
}

static void
compileHint(Compiler* compiler, KeyChord* keyChord)
{
    assert(compiler), assert(keyChord);
    if (compiler->panicMode) return;

    String result = {0};
    initString(&result);
    Modifiers* mods = &keyChord->key.mods;
    if (mods->ctrl) appendToString(&result, "C-", 2);
    if (mods->alt) appendToString(&result, "A-", 2);
    if (mods->hyper) appendToString(&result, "H-", 2);
    if (mods->shift) appendToString(&result, "S-", 2);
    if (keyChord->key.repr) appendToString(&result, keyChord->key.repr, keyChord->key.len);
    if (compiler->delimiter) appendToString(&result, compiler->delimiter, compiler->delimiterLen);
    if (keyChord->description) appendToString(
        &result, keyChord->description, strlen(keyChord->description)
    );
    keyChord->hint = result.string;
    disownString(&result);
}

static KeyChord*
compileFromPseudoChords(Compiler* compiler, KeyChord** dest)
{
    assert(compiler), assert(dest);

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
        compileKeyFromToken(&pseudo->keyToken, &chord->key);
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
    assert(compiler), assert(menu);

    if (compiler->debug) debugPrintScannedTokenHeader();

    advanceCompiler(compiler);
    while (!compilerIsAtEnd(compiler))
    {
        compileKeyChord(compiler);
    }

    if (compiler->sort) sortPseudoChordArray(compiler->chords.chords, compilerGetIndex(compiler));
    compileNullKeyChord(compiler);

    if (compiler->hadError)
    {
        debugMsg(menu->debug, "Compiler had error. Returning early.");
        freePseudoChord(&compiler->chord);
        freePseudoChordArray(&compiler->chords);
        return NULL;
    }

    menu->keyChords = compileFromPseudoChords(compiler, &menu->keyChordsHead);

    if (compiler->debug)
    {
        debugPrintScannedTokenFooter();
        disassembleKeyChords(menu->keyChordsHead, 0);
    }

    return compiler->hadError ? NULL : menu->keyChords;
}

void
initCompiler(const Menu* menu, Compiler* compiler, char *source, const char *filepath)
{
    assert(compiler), assert(source), assert(filepath);

    initScanner(&compiler->scanner, source, filepath);
    compiler->hadError = false;
    compiler->panicMode = false;
    compiler->sort = menu->sort;
    compiler->debug = menu->debug;
    compiler->delimiter = menu->delimiter;
    compiler->delimiterLen = strlen(menu->delimiter);
    makeNullPseudoChord(&compiler->nullPseudoChord);
    initPseudoChord(&compiler->chord);
    initPseudoChordArray(&compiler->chords);
    compiler->chordsDest = &compiler->chords;
    compiler->source = source;
}
