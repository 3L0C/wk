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
#include "common/menu.h"
#include "common/string.h"

/* local includes */
#include "compiler.h"
#include "debug.h"
#include "scanner.h"
#include "token.h"

typedef struct
{
    Key key;
    Array desc;
    Array cmd;
    Array before;
    Array after;
    Array chords;
    ChordFlag flags;
} PseudoChord;

static void pseudoChordArrayFree(Array* arr);

static int
pseudoChordCompare(const void* a, const void* b)
{
    assert(a), assert(b);

    PseudoChord* aChord = (PseudoChord*)a;
    PseudoChord* bChord = (PseudoChord*)b;

    if (chordFlagIsActive(aChord->flags, FLAG_IGNORE_SORT)) return 0;
    if (chordFlagIsActive(bChord->flags, FLAG_IGNORE_SORT)) return 0;

    const Key* aKey = &aChord->key;
    const Key* bKey = &bChord->key;

    bool aHasMods = modifierHasAnyActive(aKey->mods);
    bool bHasMods = modifierHasAnyActive(bKey->mods);

    if (aHasMods == bHasMods) return stringCompare(&aKey->repr, &bKey->repr);
    if (aHasMods) return 1;
    return -1;
}

static void
pseudoChordArraySort(Array* chords)
{
    assert(chords);

    qsort(ARRAY_AS(chords, PseudoChord), arrayLength(chords), sizeof(PseudoChord), pseudoChordCompare);
}

static void
pseudoChordInit(PseudoChord* chord)
{
    assert(chord);

    keyInit(&chord->key);
    chord->desc = ARRAY_INIT(Token);
    chord->cmd = ARRAY_INIT(Token);
    chord->before = ARRAY_INIT(Token);
    chord->after = ARRAY_INIT(Token);
    chord->chords = ARRAY_INIT(PseudoChord);
    chord->flags = chordFlagInit();
}

static void
pseudoChordFree(PseudoChord* chord)
{
    assert(chord);

    arrayFree(&chord->desc);
    arrayFree(&chord->cmd);
    arrayFree(&chord->before);
    arrayFree(&chord->after);
    pseudoChordArrayFree(&chord->chords);
    pseudoChordInit(chord);
}

static void
pseudoChordArrayFree(Array* arr)
{
    assert(arr);

    forEach(arr, PseudoChord, chord)
    {
        pseudoChordFree(chord);
    }

    arrayFree(arr);
}

static void compileKeyChord(Compiler* compiler);
static void compilePrefix(Compiler* compiler, PseudoChord* chord);

static void
errorAt(Compiler* compiler, Token* token, const char* fmt, ...)
{
    assert(compiler), assert(token), assert(fmt);
    if (compiler->panicMode) return;

    compiler->panicMode = true;
    compiler->hadError = true;

    va_list ap;
    va_start(ap, fmt);
    tokenErrorAt(token, compiler->scanner->filepath, fmt, ap);
    va_end(ap);
}

static void
errorAtCurrent(Compiler* compiler, const char* fmt, ...)
{
    assert(compiler), assert(fmt);
    if (compiler->panicMode) return;

    va_list ap;
    va_start(ap, fmt);
    errorAt(compiler, &compiler->current, fmt, ap);
    va_end(ap);

    compiler->panicMode = true;
    compiler->hadError = true;
}

static TokenType
advance(Compiler* compiler)
{
    assert(compiler);

    tokenCopy(&compiler->current, &compiler->previous);

    while (true)
    {
        scannerGetTokenForCompiler(compiler->scanner, &compiler->current);
        if (compiler->debug) disassembleToken(&compiler->current);
        if (compiler->current.type != TOKEN_ERROR) break;

        errorAtCurrent(compiler, compiler->current.message);
    }

    return compiler->current.type;
}

static void
appendToDest(Compiler* compiler, const PseudoChord* chord)
{
    assert(compiler), assert(chord);

    arrayAppend(compiler->dest, chord);
}

static bool
check(Compiler* compiler, TokenType type)
{
    assert(compiler);
    return compiler->current.type == type;
}

static bool
compilerIsAtEnd(Compiler* compiler)
{
    assert(compiler);
    return scannerIsAtEnd(compiler->scanner);
}

static bool
consume(Compiler* compiler, TokenType type, const char* message)
{
    assert(compiler), assert(message);

    if (compiler->current.type == type)
    {
        advance(compiler);
        return true;
    }

    errorAtCurrent(compiler, message);
    return false;
}

static Token*
currentToken(Compiler* compiler)
{
    assert(compiler);
    return &compiler->current;
}

static TokenType
currentType(Compiler* compiler)
{
    assert(compiler);
    return compiler->current.type;
}

static Array*
getDest(Compiler* compiler)
{
    assert(compiler);
    return compiler->dest;
}

static size_t
getIndex(Compiler* compiler)
{
    assert(compiler), assert(!arrayIsEmpty(compiler->dest));
    return arrayLength(compiler->dest) - 1;
}

static size_t
getNextIndex(Compiler* compiler)
{
    assert(compiler);
    return getIndex(compiler) + 1;
}

static SpecialKey
getSpecial(Compiler* compiler)
{
    assert(compiler);

    return compiler->current.special;
}

static bool
match(Compiler* compiler, TokenType type)
{
    assert(compiler);

    if (!compilerIsAtEnd(compiler) && !check(compiler, type)) return false;
    advance(compiler);
    return true;
}

static Token*
previousToken(Compiler* compiler)
{
    assert(compiler);
    return &compiler->previous;
}

static void
compileMod(Compiler* compiler, Key* key, Token* token)
{
    assert(compiler), assert(key), assert(token), assert(tokenIsModType(token->type));
    if (compiler->panicMode) return;

    switch (token->type)
    {
    case TOKEN_MOD_CTRL: key->mods |= MOD_CTRL; break;
    case TOKEN_MOD_META: key->mods |= MOD_META; break;
    case TOKEN_MOD_HYPER: key->mods |= MOD_HYPER; break;
    case TOKEN_MOD_SHIFT: key->mods |= MOD_SHIFT; break;
    default: break;
    }
}

static void
compileMods(Compiler* compiler, Key* key)
{
    assert(compiler), assert(key);
    if (compiler->panicMode) return;

    TokenType type = currentType(compiler);
    while (tokenIsModType(type))
    {
        compileMod(compiler, key, currentToken(compiler));
        type = advance(compiler);
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

static void
compileKeyToString(Compiler* compiler, Key* key)
{
    assert(compiler), assert(key);
    if (compiler->panicMode) return;

    Token* token = currentToken(compiler);
    if (token->type == TOKEN_SPECIAL_KEY)
    {
        key->special = getSpecial(compiler);
        stringAppendCString(&key->repr, specialKeyGetRepr(key->special));
    }
    else
    {
        stringAppend(&key->repr, token->start, token->length);
    }
}

static void
compileKey(Compiler* compiler, Key* key)
{
    assert(compiler), assert(key);
    if (compiler->panicMode) return;

    if (!isKey(compiler)) return errorAtCurrent(compiler, "Expected key or special.");

    compileKeyToString(compiler, key);
    consume(compiler, currentType(compiler), "Expected key or special.");
}

static void
compileDescriptionTokens(Compiler* compiler, Array* desc)
{
    assert(compiler), assert(desc);
    if (compiler->panicMode) return;

    if (!check(compiler, TOKEN_DESC_INTERP) &&
        !check(compiler, TOKEN_DESCRIPTION))
    {
        errorAtCurrent(compiler, "Expected description.");
        return;
    }

    while (!check(compiler, TOKEN_EOF))
    {
        Token* token = currentToken(compiler);
        switch (token->type)
        {
        case TOKEN_THIS_KEY: /* FALLTHROUGH */
        case TOKEN_INDEX:
        case TOKEN_INDEX_ONE:
        case TOKEN_DESC_INTERP:
        case TOKEN_DESCRIPTION: arrayAppend(desc, token); break;
        default: errorAtCurrent(compiler, "Malfromed description."); return;
        }
        if (check(compiler, TOKEN_DESCRIPTION)) break;
        advance(compiler);
    }

    consume(compiler, TOKEN_DESCRIPTION, "Expect description.");
    return;
}

static bool
compileCommandTokens(Compiler* compiler, Array* cmd, bool inChordArray, const char* message)
{
    assert(compiler), assert(cmd), assert(message);
    if (compiler->panicMode) return false;
    if (inChordArray && check(compiler, TOKEN_RIGHT_PAREN)) return false;

    if (!check(compiler, TOKEN_COMM_INTERP) &&
        !check(compiler, TOKEN_COMMAND))
    {
        errorAtCurrent(compiler, message);
        return false;
    }

    while (!check(compiler, TOKEN_EOF))
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
        case TOKEN_COMMAND: arrayAppend(cmd, token); break;
        default: errorAtCurrent(compiler, "Malfromed command."); return false;
        }
        if (check(compiler, TOKEN_COMMAND)) break;
        advance(compiler);
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
        return compileCommandTokens(
            compiler, &chord->before,
            false, "Expected command after '^before' hook."
        );
    }
    case TOKEN_AFTER:
    {
        consume(compiler, TOKEN_AFTER, "Expected '^after' hook.");
        return compileCommandTokens(
            compiler, &chord->after,
            false, "Expected command after '^after' hook."
        );
    }
    case TOKEN_SYNC_BEFORE:
    {
        consume(compiler, TOKEN_SYNC_BEFORE, "Expected '^sync-before' hook.");
        chord->flags |= FLAG_SYNC_BEFORE;
        return compileCommandTokens(
            compiler, &chord->before,
            false, "Expected command after '^sync-before' hook."
        );
    }
    case TOKEN_SYNC_AFTER:
    {
        consume(compiler, TOKEN_SYNC_AFTER, "Expected '^sync-after' hook.");
        chord->flags |= FLAG_SYNC_AFTER;
        return compileCommandTokens(
            compiler, &chord->after,
            false, "Expected command after '^sync-after' hook."
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
    case TOKEN_KEEP: chord->flags |= FLAG_KEEP; return true;
    case TOKEN_CLOSE: chord->flags |= FLAG_CLOSE; return true;
    case TOKEN_INHERIT: chord->flags |= FLAG_INHERIT; return true;
    case TOKEN_IGNORE: chord->flags |= FLAG_IGNORE; return true;
    case TOKEN_IGNORE_SORT: chord->flags |= FLAG_IGNORE_SORT; return true;
    case TOKEN_UNHOOK: chord->flags |= FLAG_UNHOOK; return true;
    case TOKEN_DEFLAG: chord->flags |= FLAG_DEFLAG; return true;
    case TOKEN_NO_BEFORE: chord->flags |= FLAG_NO_BEFORE; return true;
    case TOKEN_NO_AFTER: chord->flags |= FLAG_NO_AFTER; return true;
    case TOKEN_WRITE: chord->flags |= FLAG_WRITE; return true;
    case TOKEN_EXECUTE: chord->flags |= FLAG_EXECUTE; return true;
    case TOKEN_SYNC_CMD: chord->flags |= FLAG_SYNC_COMMAND; return true;
    default: return false;
    }
}

static void
compileHooksAndFlags(Compiler* compiler, PseudoChord* chord)
{
    assert(compiler), assert(chord);
    if (compiler->panicMode) return;

    TokenType type = currentType(compiler);
    while (!compilerIsAtEnd(compiler))
    {
        if (!compileHook(compiler, chord, type) && !compileFlag(compiler, chord, type)) return;

        /* Hooks consume their own token and their command
         * which leaves them pointing at the next token. */
        type = tokenIsHookType(type) ? currentType(compiler) : advance(compiler);
    }
}

static void
compileMissingKeyChordInfo(Compiler* compiler, const PseudoChord* from, PseudoChord* to)
{
    assert(compiler), assert(from), assert(to);

    if (!chordFlagHasAnyActive(to->flags)) to->flags = from->flags;
    if (arrayLength(&to->desc) == 0) to->desc = arrayCopy(&from->desc);
    if (arrayLength(&to->cmd) == 0) to->cmd = arrayCopy(&from->cmd);
    if (arrayLength(&to->before) == 0) to->before = arrayCopy(&from->before);
    if (arrayLength(&to->after) == 0) to->after = arrayCopy(&from->after);
}

static void
compileImplicitChordArray(Compiler* compiler, PseudoChord* dummy)
{
    assert(compiler);

    if (!check(compiler, TOKEN_DESCRIPTION) && !check(compiler, TOKEN_DESC_INTERP))
    {
        errorAtCurrent(compiler, "Expected description, or description expressions after '...'.");
        return;
    }

    compileDescriptionTokens(compiler, &dummy->desc);
    compileHooksAndFlags(compiler, dummy);
    compileCommandTokens(compiler, &dummy->cmd, false, "Expected command.");

    forEach(&compiler->implicitKeys, const Key, key)
    {
        PseudoChord chord = {0};
        pseudoChordInit(&chord);
        keyCopy(key, &chord.key);
        compileMissingKeyChordInfo(compiler, dummy, &chord);
        arrayAppend(compiler->dest, &chord);
    }

    pseudoChordFree(dummy);
}

static void
compileChordArray(Compiler* compiler)
{
    assert(compiler);

    if (!isKey(compiler) &&
        !tokenIsModType(currentType(compiler)) &&
        !check(compiler, TOKEN_LEFT_PAREN))
    {
        errorAtCurrent(compiler, "Expect modifier, key, or chord expression after '['.");
        return;
    }

    size_t arrayStart = getNextIndex(compiler);

    while (!compilerIsAtEnd(compiler) &&
           !check(compiler, TOKEN_RIGHT_BRACKET))
    {
        if (!isKey(compiler) &&
            !tokenIsModType(currentType(compiler)) &&
            !check(compiler, TOKEN_LEFT_PAREN))
        {
            errorAtCurrent(
                compiler,
                "Chord arrays may only contain modifiers, keys, and chord expressions."
            );
            compiler->hadError = true;
            return;
        }

        PseudoChord chord = {0};
        pseudoChordInit(&chord);

        if (match(compiler, TOKEN_LEFT_PAREN))
        {
            compileMods(compiler, &chord.key);
            compileKey(compiler, &chord.key);
            compileDescriptionTokens(compiler, &chord.desc);
            compileHooksAndFlags(compiler, &chord);
            compileCommandTokens(compiler, &chord.cmd, true, "Expected command.");
            consume(compiler, TOKEN_RIGHT_PAREN, "Expect closing parenthesis after '('.");
        }
        else
        {
            compileMods(compiler, &chord.key);
            compileKey(compiler, &chord.key);
        }

        appendToDest(compiler, &chord);
        pseudoChordFree(&chord);
    }

    consume(compiler, TOKEN_RIGHT_BRACKET, "Expect ']' after chord array.");
    PseudoChord dummy = {0};
    pseudoChordInit(&dummy);

    compileDescriptionTokens(compiler, &dummy.desc);
    compileHooksAndFlags(compiler, &dummy);
    compileCommandTokens(compiler, &dummy.cmd, false, "Expected command.");

    /* Write chords in chord array to destination */
    forEachFrom(getDest(compiler), PseudoChord, chord, arrayStart)
    {
        compileMissingKeyChordInfo(compiler, &dummy, chord);
    }

    pseudoChordFree(&dummy);

    return;
}

static void
compileChord(Compiler* compiler)
{
    assert(compiler);

    PseudoChord dummy = {0};
    pseudoChordInit(&dummy);

    compileMods(compiler, &dummy.key);
    if (match(compiler, TOKEN_ELLIPSIS)) return compileImplicitChordArray(compiler, &dummy);
    compileKey(compiler, &dummy.key);
    compileDescriptionTokens(compiler, &dummy.desc);
    compileHooksAndFlags(compiler, &dummy);

    /* Prefix */
    if (check(compiler, TOKEN_LEFT_BRACE)) return compilePrefix(compiler, &dummy);

    compileCommandTokens(compiler, &dummy.cmd, false, "Expected command.");

    /* Check for brace after command */
    if (check(compiler, TOKEN_LEFT_BRACE))
    {
        errorAtCurrent(compiler, "Expected end of key chord after command but got '{'.");
        return;
    }

    appendToDest(compiler, &dummy);
    pseudoChordFree(&dummy);
}

static void
setBeforeHook(PseudoChord* parent, PseudoChord* child)
{
    assert(parent), assert(child);

    /* Children that opt out do not inherit */
    if (chordFlagIsActive(child->flags, FLAG_NO_BEFORE) || arrayLength(&parent->before) == 0) return;

    arrayFree(&child->before);
    child->before = arrayCopy(&parent->before);

    /* set syncBefore flag */
    if (chordFlagIsActive(parent->flags, FLAG_SYNC_BEFORE)) child->flags |= FLAG_SYNC_BEFORE;
}

static void
setAfterHook(PseudoChord* parent, PseudoChord* child)
{
    assert(parent), assert(child);

    /* Children that opt out do not inherit */
    if (chordFlagIsActive(child->flags, FLAG_NO_AFTER) || arrayLength(&parent->after) == 0) return;

    arrayFree(&child->after);
    child->after = arrayCopy(&parent->after);

    /* set syncAfter flag */
    if (chordFlagIsActive(parent->flags, FLAG_SYNC_AFTER)) child->flags |= FLAG_SYNC_AFTER;
}

static void
setHooks(PseudoChord* parent, PseudoChord* child)
{
    assert(parent), assert(child);

    /* Children that opt out do not inherit */
    if (chordFlagIsActive(child->flags, FLAG_UNHOOK)) return;

    setBeforeHook(parent, child);
    setAfterHook(parent, child);
}

static ChordFlag
setFlags(ChordFlag parent, ChordFlag child)
{
    /* Children that opt out do not inherit */
    if (chordFlagIsActive(child, FLAG_DEFLAG)) return child;

    if (!chordFlagIsActive(child, FLAG_CLOSE) && chordFlagIsActive(parent, FLAG_KEEP))
    {
        child |= FLAG_KEEP;
    }
    if (!chordFlagIsActive(child, FLAG_EXECUTE) && chordFlagIsActive(parent, FLAG_WRITE))
    {
        child |= FLAG_WRITE;
    }
    if (chordFlagIsActive(parent, FLAG_SYNC_COMMAND))
    {
        child |= FLAG_SYNC_COMMAND;
    }

    return child;
}

static void
setHooksAndFlags(PseudoChord* parent, Array* children)
{
    assert(parent), assert(children);

    forEach(children, PseudoChord, child)
    {
        if (chordFlagIsActive(child->flags, FLAG_IGNORE)) continue;
        if (!arrayIsEmpty(&child->chords) && !chordFlagIsActive(child->flags, FLAG_INHERIT)) continue;

        setHooks(parent, child);
        child->flags = setFlags(parent->flags, child->flags);
        if (!arrayIsEmpty(&child->chords))
        {
            setHooksAndFlags(child, &child->chords);
        }
    }
}

static void
compilePrefix(Compiler* compiler, PseudoChord* chord)
{
    assert(compiler), assert(chord);

    /* Backup information */
    Array* previousDest = getDest(compiler);

    /* advance */
    appendToDest(compiler, chord);
    PseudoChord* parent = ARRAY_GET_LAST(previousDest, PseudoChord);
    pseudoChordFree(chord);
    Array* children = &parent->chords;
    compiler->dest = children;

    /* Compile children */
    while (!compilerIsAtEnd(compiler) && !check(compiler, TOKEN_RIGHT_BRACE))
    {
        compileKeyChord(compiler);
    }
    consume(compiler, TOKEN_RIGHT_BRACE, "Expect '}' after prefix.");

    if (arrayIsEmpty(children))
    {
        errorAt(compiler, previousToken(compiler), "No key chords set for prefix.");
        compiler->hadError = true;
    }

    /* end prefix */
    setHooksAndFlags(parent, getDest(compiler));
    compiler->dest = previousDest;
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
            while (!compilerIsAtEnd(compiler) && !check(compiler, TOKEN_COMMAND))
            {
                advance(compiler);
            }
            consume(compiler, TOKEN_COMMAND, "Expected command.");
            if (!tokenIsModType(currentType(compiler)) && !isKey(compiler))
            {
                while (!compilerIsAtEnd(compiler))
                {
                    if (tokenIsModType(currentType(compiler)) || isKey(compiler)) break;
                    advance(compiler);
                }
            }
            return;
        }
        /* Return on prefix start '{' */
        case TOKEN_LEFT_BRACE: return;
        default: advance(compiler); break;
        }
    }
}

static void
compileKeyChord(Compiler* compiler)
{
    assert(compiler);

    if (compiler->panicMode) synchronize(compiler);
    if (compilerIsAtEnd(compiler)) return;

    if (match(compiler, TOKEN_ELLIPSIS))
    {
        PseudoChord dummy = {0};
        pseudoChordInit(&dummy);
        compileImplicitChordArray(compiler, &dummy);
    }
    else if (match(compiler, TOKEN_LEFT_BRACKET))
    {
        compileChordArray(compiler);
    }
    else
    {
        compileChord(compiler);
    }
}

static void
compileStringFromKey(String* result, const Key* key)
{
    assert(result), assert(key);

    stringAppendString(result, &key->repr);
}

static void
compileDescriptionWithState(Compiler* compiler, String* dest, TokenType type, const String* desc)
{
    assert(compiler), assert(dest), assert(desc);


    StringCase state;
    switch (type)
    {
    case TOKEN_THIS_DESC_UPPER_FIRST: state = STRING_CASE_UPPER_FIRST; break;
    case TOKEN_THIS_DESC_LOWER_FIRST: state = STRING_CASE_LOWER_FIRST; break;
    case TOKEN_THIS_DESC_UPPER_ALL: state = STRING_CASE_UPPER_ALL; break;
    case TOKEN_THIS_DESC_LOWER_ALL: state = STRING_CASE_LOWER_ALL; break;
    default:
    {
        errorMsg("Got unexpected token type to `compileDescriptionWithState`.");
        stringAppendString(dest, desc);
        return;
    }
    }

    stringAppendStringWithState(compiler->arena, dest, desc, state);
}

static void
compileStringFromToken(Compiler* compiler, Token* token, KeyChord* to, String* dest, size_t index)
{
    assert(compiler), assert(token), assert(to), assert(dest);

    switch (token->type)
    {
    case TOKEN_THIS_KEY: compileStringFromKey(dest, &to->key); break;
    case TOKEN_THIS_DESC:
    {
        if (!stringIsEmpty(&to->description)) stringAppendString(dest, &to->description);
        break;
    }
    case TOKEN_THIS_DESC_UPPER_FIRST: /* FALLTHROUGH */
    case TOKEN_THIS_DESC_LOWER_FIRST:
    case TOKEN_THIS_DESC_UPPER_ALL:
    case TOKEN_THIS_DESC_LOWER_ALL:
    {
        if (!stringIsEmpty(&to->description))
        {
            compileDescriptionWithState(compiler, dest, token->type, &to->description);
        }
        break;
    }
    case TOKEN_INDEX: stringAppendUInt32(compiler->arena, dest, index); break;
    case TOKEN_INDEX_ONE: stringAppendUInt32(compiler->arena, dest, index + 1); break;
    case TOKEN_DESC_INTERP: /* FALLTHROUGH */
    case TOKEN_DESCRIPTION: stringAppendEscString(compiler->arena, dest, token->start, token->length); break;
    case TOKEN_COMM_INTERP: /* FALLTHROUGH */
    case TOKEN_COMMAND: stringAppend(dest, token->start, token->length); break;
    default:
    {
        errorMsg(
            "Got unexpected token when compiling token array: '%s'.",
            tokenGetLiteral(token->type)
        );
        break;
    }
    }
}

static void
compileStringFromTokens(Compiler* compiler, Array* tokens, KeyChord* to, String* dest, size_t index)
{
    assert(compiler), assert(tokens), assert(to), assert(dest);

    *dest = stringInit();

    forEach(tokens, Token, token)
    {
        compileStringFromToken(compiler, token, to, dest, index);
    }

    stringRtrim(dest);
}

static Array*
compileFromPseudoChords(Compiler* compiler, Array* dest)
{
    assert(compiler), assert(dest);

    Array* root = compiler->dest;
    forEach(root, PseudoChord, chord)
    {
        KeyChord* keyChord = ARRAY_APPEND_SLOT(dest, KeyChord);
        /* Key */
        keyCopy(&chord->key, &keyChord->key);
        /* Description */
        compileStringFromTokens(compiler, &chord->desc, keyChord, &keyChord->description, iter.index);
        /* Hooks */
        compileStringFromTokens(compiler, &chord->before, keyChord, &keyChord->before, iter.index);
        compileStringFromTokens(compiler, &chord->after, keyChord, &keyChord->after, iter.index);
        keyChord->flags = chord->flags;
        /* Command */
        compileStringFromTokens(compiler, &chord->cmd, keyChord, &keyChord->command, iter.index);
        if (!arrayIsEmpty(&chord->chords))
        {
            Array* children = &chord->chords;
            compiler->dest = children;
            compileFromPseudoChords(compiler, &keyChord->keyChords);
            compiler->dest = root;
        }
    }

    pseudoChordArrayFree(compiler->dest);
    return dest;
}

static bool
compileImplicitChordArrayKeys(Compiler* compiler, Menu* menu)
{
    assert(compiler), assert(menu);

    scannerInit(&compiler->implicitArrayKeysScanner, menu->implicitArrayKeys, ".");
    compiler->scanner = &compiler->implicitArrayKeysScanner;

    while (!compilerIsAtEnd(compiler))
    {
        switch (advance(compiler))
        {
        case TOKEN_SPECIAL_KEY: /* FALLTHROUGH */
        case TOKEN_KEY:
        case TOKEN_MOD_CTRL:
        case TOKEN_MOD_META:
        case TOKEN_MOD_HYPER:
        case TOKEN_MOD_SHIFT:
        {
            Key* key = ARRAY_APPEND_SLOT(&compiler->implicitKeys, Key);
            keyInit(key);
            compileMods(compiler, key);
            compileKey(compiler, key);
            break;
        }
        default:
        {
            errorAtCurrent(compiler, "Expected modifier, special, or normal key.");
            tokenInit(currentToken(compiler));
            compiler->scanner = &compiler->sourceScanner;
            return false;
        }
        }
    }

    tokenInit(currentToken(compiler));
    compiler->scanner = &compiler->sourceScanner;
    return true;
}

static void
compilerFree(Compiler* compiler)
{
    assert(compiler);

    if (compiler->chords != NULL) pseudoChordArrayFree(compiler->chords);
    arrayFree(&compiler->implicitKeys);
}

Array*
compileKeyChords(Compiler* compiler, Menu* menu)
{
    assert(compiler), assert(menu);

    if (compiler->debug) debugPrintScannedTokenHeader();

    if (!compileImplicitChordArrayKeys(compiler, menu))
    {
        errorMsg("Could not compile chord array keys: '%s'.", menu->implicitArrayKeys);
        compilerFree(compiler);
        return NULL;
    }

    Array chords = ARRAY_INIT(PseudoChord);
    compiler->chords = compiler->dest = &chords;

    advance(compiler);
    while (!compilerIsAtEnd(compiler))
    {
        compileKeyChord(compiler);
    }

    if (compiler->hadError)
    {
        compilerFree(compiler);
        return NULL;
    }

    if (compiler->sort) pseudoChordArraySort(compiler->chords);
    menu->keyChords = compileFromPseudoChords(compiler, menu->keyChordsHead);

    if (compiler->debug)
    {
        debugPrintScannedTokenFooter();
        disassembleKeyChordArray(menu->keyChordsHead, 0);
    }

    compilerFree(compiler);
    return compiler->hadError ? NULL : menu->keyChords;
}

void
initCompiler(Compiler* compiler, Menu* menu, char* source, const char* filepath)
{
    assert(compiler), assert(source), assert(filepath);

    scannerInit(&compiler->sourceScanner, source, filepath);
    tokenInit(currentToken(compiler));
    tokenInit(previousToken(compiler));
    compiler->implicitKeys = ARRAY_INIT(KeyChord);
    compiler->dest = NULL;
    compiler->chords = NULL;
    compiler->arena = &menu->arena;
    compiler->delimiter = menu->delimiter;
    compiler->source = source;
    compiler->delimiterLen = strlen(menu->delimiter);
    compiler->hadError = false;
    compiler->panicMode = false;
    compiler->sort = menu->sort;
    compiler->debug = menu->debug;
}
