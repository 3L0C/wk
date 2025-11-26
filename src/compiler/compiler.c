#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* common includes */
#include "common/array.h"
#include "common/common.h"
#include "common/debug.h"
#include "common/key_chord.h"
#include "common/key_chord_def.h"
#include "common/menu.h"
#include "common/property.h"
#include "common/stack.h"
#include "common/string.h"

/* local includes */
#include "compiler.h"
#include "debug.h"
#include "scanner.h"
#include "token.h"

/* Forward declarations */
static void initKeyChordArrayProps(KeyChord* chord);
static void freeKeyChordArrayProps(KeyChord* chord);
static void compileDescriptionWithState(Compiler* compiler, String* dest, TokenType type, const String* desc);
static bool propertyHasContent(const Property* prop);
static void compilePropertyString(Compiler* compiler, KeyChord* chord, KeyChordPropId propId, size_t index);
static void resolveChordTokenArrays(Compiler* compiler, KeyChord* chord, size_t sourceIndex);

/* Key categorization for emacs which-key style sorting */
typedef enum
{
    KEY_CAT_SPECIAL, /* Special keys (SPC, ESC, F1, etc.) */
    KEY_CAT_NUMBER,  /* 0-9 */
    KEY_CAT_SYMBOL,  /* Punctuation and special characters */
    KEY_CAT_LETTER,  /* a-z, A-Z */
} KeyCategory;

static KeyCategory
getKeyCategory(const Key* key)
{
    assert(key);

    /* Special keys have special != SPECIAL_KEY_NONE */
    if (key->special != SPECIAL_KEY_NONE) return KEY_CAT_SPECIAL;

    /* Get first character from repr for normal keys */
    StringIterator iter = stringIteratorMake(&key->repr);
    char           c    = stringIteratorPeek(&iter);

    if (c >= '0' && c <= '9') return KEY_CAT_NUMBER;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return KEY_CAT_LETTER;
    return KEY_CAT_SYMBOL;
}

/* Helper for initializing properties as Token arrays */
static inline void
initPropAsTokenArray(KeyChord* chord, KeyChordPropId id)
{
    Property* prop = keyChordProperty(chord, id);
    PROP_SET_TYPE(prop, ARRAY);
    *propertyAsArray(prop) = ARRAY_INIT(Token);
}

static int
keyChordCompareForSort(const void* a, const void* b)
{
    assert(a), assert(b);

    const KeyChord* aChord = (const KeyChord*)a;
    const KeyChord* bChord = (const KeyChord*)b;

    /* Existing FLAG_IGNORE_SORT handling */
    if (chordFlagIsActive(aChord->flags, FLAG_IGNORE_SORT)) return 0;
    if (chordFlagIsActive(bChord->flags, FLAG_IGNORE_SORT)) return 0;

    const Key* aKey = &aChord->key;
    const Key* bKey = &bChord->key;

    /* 1. Primary sort: by category (emacs which-key style) */
    KeyCategory catA = getKeyCategory(aKey);
    KeyCategory catB = getKeyCategory(bKey);
    if (catA != catB) return (int)catA - (int)catB;

    /* 2. Secondary sort: unmodified before modified */
    bool aHasMods = modifierHasAnyActive(aKey->mods);
    bool bHasMods = modifierHasAnyActive(bKey->mods);
    if (aHasMods != bHasMods) return aHasMods ? 1 : -1;

    /* 3. Tertiary sort: within category */
    /* For letters: group same letters together (a, A, b, B), lowercase before uppercase */
    if (catA == KEY_CAT_LETTER)
    {
        StringIterator iterA = stringIteratorMake(&aKey->repr);
        StringIterator iterB = stringIteratorMake(&bKey->repr);
        char           cA    = stringIteratorPeek(&iterA);
        char           cB    = stringIteratorPeek(&iterB);

        /* Compare case-insensitively first to group same letters */
        char lowerA = (cA >= 'A' && cA <= 'Z') ? cA + 32 : cA;
        char lowerB = (cB >= 'A' && cB <= 'Z') ? cB + 32 : cB;

        if (lowerA != lowerB) return lowerA - lowerB;

        /* Same letter: lowercase comes before uppercase */
        bool aIsLower = (cA >= 'a' && cA <= 'z');
        bool bIsLower = (cB >= 'a' && cB <= 'z');
        if (aIsLower != bIsLower) return aIsLower ? -1 : 1;
    }

    /* Default: string comparison (alphabetical for special keys,
     * numeric order for digits, ASCII for symbols) */
    return stringCompare(&aKey->repr, &bKey->repr);
}

static void
keyChordArraySort(Array* chords)
{
    assert(chords);

    qsort(
        ARRAY_AS(chords, KeyChord),
        arrayLength(chords),
        sizeof(KeyChord),
        keyChordCompareForSort);
}

/* Initialize a KeyChord for compilation phase (with Token arrays in properties) */
static void
initKeyChordArrayProps(KeyChord* chord)
{
    assert(chord);

    keyChordInit(chord);

    /* Initialize properties as Token arrays based on their compile type */
#define KC_PROP(id, name, accessor, rt, ct)         \
    if (keyChordCompileType(id) == PROP_TYPE_ARRAY) \
        initPropAsTokenArray(chord, id);
    KEY_CHORD_PROP_LIST
#undef KC_PROP
}

/* Forward declaration */
static void freeKeyChordArrayPropsRecursive(Array* arr);

/* Free a KeyChord used during compilation phase (with Token arrays) */
static void
freeKeyChordArrayProps(KeyChord* chord)
{
    assert(chord);

    keyFree(&chord->key);

    /* Free all properties (propertyFree handles both Array and String types) */
    for (size_t i = 0; i < KC_PROP_COUNT; i++)
    {
        propertyFree(&chord->props[i]);
    }

    freeKeyChordArrayPropsRecursive(&chord->keyChords);
}

static int
compareSize_t(const void* a, const void* b)
{
    size_t x = *(const size_t*)a;
    size_t y = *(const size_t*)b;
    return (x > y) - (x < y);
}

static void
deduplicateKeyChordArray(Array* chords)
{
    assert(chords);

    if (arrayIsEmpty(chords)) return;

    Stack stack = STACK_INIT(size_t);

    /* Phase 1: Find duplicates and swap last definition to first position */
    forRange(chords, KeyChord, outerChord, 0, arrayLength(chords) - 1)
    {
        ArrayIterator* outerIter = &iter;
        size_t         swapIdx   = outerIter->index;
        forEachFrom(chords, KeyChord, innerChord, outerIter->index + 1)
        {
            if (keyIsEqual(&outerChord->key, &innerChord->key))
            {
                /* Mark the later position for removal */
                stackPush(&stack, &iter.index);
                /* Update the swap index */
                swapIdx = iter.index;
            }
        }

        /* Swap so last definition moves to earliest position */
        arraySwap(chords, outerIter->index, swapIdx);
    }

    /* Phase 2: Sort the duplicates stack */
    qsort(stack.data, stack.length, stack.elementSize, compareSize_t);

    /* Phase 3: Remove duplicates in reverse order to avoid index invalidation */
    while (!stackIsEmpty(&stack))
    {
        size_t    index = *(STACK_PEEK(&stack, size_t));
        KeyChord* dup   = ARRAY_GET(chords, KeyChord, index);
        freeKeyChordArrayProps(dup);
        arrayRemove(chords, index);

        /* Skip duplicate indices in stack i.e., [2, 4, 5, 5] */
        while (!stackIsEmpty(&stack) && index == *(STACK_PEEK(&stack, size_t)))
        {
            stackPop(&stack);
        }
    }

    stackFree(&stack);

    /* Phase 4: Recursively deduplicate nested chords */
    forEach(chords, KeyChord, chord)
    {
        if (!arrayIsEmpty(&chord->keyChords))
        {
            deduplicateKeyChordArray(&chord->keyChords);
        }
    }
}

static void
freeKeyChordArrayPropsRecursive(Array* arr)
{
    assert(arr);

    forEach(arr, KeyChord, chord) { freeKeyChordArrayProps(chord); }

    arrayFree(arr);
}

static void compileKeyChord(Compiler* compiler);
static void compilePrefix(Compiler* compiler, KeyChord* chord);

static void
errorAt(Compiler* compiler, Token* token, const char* fmt, ...)
{
    assert(compiler), assert(token), assert(fmt);
    if (compiler->panicMode) return;

    compiler->panicMode = true;
    compiler->hadError  = true;

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
    compiler->hadError  = true;
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
appendToDest(Compiler* compiler, const KeyChord* chord)
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
compilerDestIsEmpty(Compiler* compiler)
{
    assert(compiler);
    return arrayIsEmpty(compiler->dest);
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

    if (compilerIsAtEnd(compiler) || !check(compiler, type)) return false;
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

/* Immediate resolution: resolve description tokens directly to String */
static void
compileDescription(Compiler* compiler, KeyChord* chord, size_t index)
{
    assert(compiler), assert(chord);
    if (compiler->panicMode) return;

    if (!check(compiler, TOKEN_DESC_INTERP) && !check(compiler, TOKEN_DESCRIPTION))
    {
        errorAtCurrent(compiler, "Expected description.");
        return;
    }

    Property* prop = keyChordProperty(chord, KC_PROP_DESCRIPTION);
    PROP_SET_TYPE(prop, STRING);
    String* desc = PROP_VAL(prop, as_string);
    *desc        = stringInit();

    while (!check(compiler, TOKEN_EOF))
    {
        Token* token = currentToken(compiler);
        switch (token->type)
        {
        case TOKEN_THIS_KEY: stringAppendString(desc, &chord->key.repr); break;
        case TOKEN_INDEX: stringAppendUInt32(compiler->arena, desc, index); break;
        case TOKEN_INDEX_ONE: stringAppendUInt32(compiler->arena, desc, index + 1); break;
        case TOKEN_USER_VAR:
        {
            bool found = false;
            forEach(compiler->userVars, const UserVar, var)
            {
                if (strncmp(var->key, token->start, token->length) == 0)
                {
                    stringAppendCString(desc, var->value);
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                tokenErrorAt(
                    token,
                    compiler->scanner->filepath,
                    "Undefined variable '%%(%.*s)'. Use :var \"%.*s\" \"value\" to define it.",
                    token->length,
                    token->start,
                    token->length,
                    token->start);
                compiler->panicMode = true;
                compiler->hadError  = true;
            }
            break;
        }
        case TOKEN_WRAP_CMD_INTERP:
        {
            if (!stringIsEmpty(&compiler->menu->wrapCmd))
            {
                stringAppendString(desc, &compiler->menu->wrapCmd);
            }
            break;
        }
        case TOKEN_DESC_INTERP: /* FALLTHROUGH */
        case TOKEN_DESCRIPTION: stringAppendEscString(desc, token->start, token->length); break;
        default: errorAtCurrent(compiler, "Malformed description."); return;
        }
        if (check(compiler, TOKEN_DESCRIPTION)) break;
        advance(compiler);
    }

    consume(compiler, TOKEN_DESCRIPTION, "Expect description.");
    stringRtrim(desc);
}

/* Delayed resolution: store description tokens for later resolution (dummy chords) */
static void
compileDescriptionTokens(Compiler* compiler, Array* desc)
{
    assert(compiler), assert(desc);
    if (compiler->panicMode) return;

    if (!check(compiler, TOKEN_DESC_INTERP) && !check(compiler, TOKEN_DESCRIPTION))
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
        case TOKEN_USER_VAR:
        case TOKEN_WRAP_CMD_INTERP:
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

/* Delayed mode: compile meta command (stores tokens) */
static bool
compileMetaCmdDelayed(Compiler* compiler, KeyChord* chord)
{
    assert(compiler), assert(chord);
    if (compiler->panicMode) return false;

    if (!arrayIsEmpty(keyChordArray(chord, KC_PROP_BEFORE)) ||
        !arrayIsEmpty(keyChordArray(chord, KC_PROP_AFTER)))
    {
        errorAtCurrent(compiler, "Cannot mix meta commands and hooks.");
        return false;
    }

    if (!arrayIsEmpty(keyChordArray(chord, KC_PROP_COMMAND)))
    {
        errorAtCurrent(compiler, "Cannot mix commands and meta commands.");
        return false;
    }

    switch (currentType(compiler))
    {
    case TOKEN_GOTO:
    {
        consume(compiler, TOKEN_GOTO, "Expected '@goto' meta command.");
        compileDescriptionTokens(compiler, keyChordArray(chord, KC_PROP_GOTO));
        return true;
    }
    default:
    {
        errorAtCurrent(compiler, "Got unhandled meta command.");
        return false;
    }
    }

    return false;
}

/* Immediate mode: compile meta command (resolves to String) */
static bool
compileMetaCmdImmediate(Compiler* compiler, KeyChord* chord, size_t index)
{
    assert(compiler), assert(chord);
    if (compiler->panicMode) return false;

    const Property* beforeProp = keyChordPropertyConst(chord, KC_PROP_BEFORE);
    const Property* afterProp  = keyChordPropertyConst(chord, KC_PROP_AFTER);
    if (propertyHasContent(beforeProp) || propertyHasContent(afterProp))
    {
        errorAtCurrent(compiler, "Cannot mix meta commands and hooks.");
        return false;
    }

    const Property* cmdProp = keyChordPropertyConst(chord, KC_PROP_COMMAND);
    if (propertyHasContent(cmdProp))
    {
        errorAtCurrent(compiler, "Cannot mix commands and meta commands.");
        return false;
    }

    switch (currentType(compiler))
    {
    case TOKEN_GOTO:
    {
        consume(compiler, TOKEN_GOTO, "Expected '@goto' meta command.");
        compilePropertyString(compiler, chord, KC_PROP_GOTO, index);
        return true;
    }
    default:
    {
        errorAtCurrent(compiler, "Got unhandled meta command.");
        return false;
    }
    }

    return false;
}

/* Immediate resolution: resolve command tokens directly to String */
static bool
compileCommand(Compiler* compiler, KeyChord* chord, size_t index, bool inChordArray, const char* message)
{
    assert(compiler), assert(chord), assert(message);
    if (compiler->panicMode) return false;
    if (inChordArray && check(compiler, TOKEN_RIGHT_PAREN)) return false;

    if (!check(compiler, TOKEN_COMM_INTERP) && !check(compiler, TOKEN_COMMAND))
    {
        errorAtCurrent(compiler, message);
        return false;
    }

    Property* prop = keyChordProperty(chord, KC_PROP_COMMAND);
    PROP_SET_TYPE(prop, STRING);
    String* cmd = PROP_VAL(prop, as_string);
    *cmd        = stringInit();

    const String* desc = keyChordDescriptionConst(chord);

    while (!check(compiler, TOKEN_EOF))
    {
        Token* token = currentToken(compiler);
        switch (token->type)
        {
        case TOKEN_THIS_KEY: stringAppendString(cmd, &chord->key.repr); break;
        case TOKEN_INDEX: stringAppendUInt32(compiler->arena, cmd, index); break;
        case TOKEN_INDEX_ONE: stringAppendUInt32(compiler->arena, cmd, index + 1); break;
        case TOKEN_USER_VAR:
        {
            bool found = false;
            forEach(compiler->userVars, const UserVar, var)
            {
                if (strncmp(var->key, token->start, token->length) == 0)
                {
                    stringAppendCString(cmd, var->value);
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                tokenErrorAt(
                    token,
                    compiler->scanner->filepath,
                    "Undefined variable '%%(%.*s)'. Use :var \"%.*s\" \"value\" to define it.",
                    token->length,
                    token->start,
                    token->length,
                    token->start);
                compiler->panicMode = true;
                compiler->hadError  = true;
            }
            break;
        }
        case TOKEN_WRAP_CMD_INTERP:
        {
            if (!stringIsEmpty(&compiler->menu->wrapCmd))
            {
                stringAppendString(cmd, &compiler->menu->wrapCmd);
            }
            break;
        }
        case TOKEN_THIS_DESC:
        {
            if (desc && !stringIsEmpty(desc)) stringAppendString(cmd, desc);
            break;
        }
        case TOKEN_THIS_DESC_UPPER_FIRST: /* FALLTHROUGH */
        case TOKEN_THIS_DESC_LOWER_FIRST:
        case TOKEN_THIS_DESC_UPPER_ALL:
        case TOKEN_THIS_DESC_LOWER_ALL:
        {
            if (desc && !stringIsEmpty(desc))
            {
                compileDescriptionWithState(compiler, cmd, token->type, desc);
            }
            break;
        }
        case TOKEN_COMM_INTERP: /* FALLTHROUGH */
        case TOKEN_COMMAND: stringAppend(cmd, token->start, token->length); break;
        default: errorAtCurrent(compiler, "Malformed command."); return false;
        }
        if (check(compiler, TOKEN_COMMAND)) break;
        advance(compiler);
    }

    stringRtrim(cmd);
    return consume(compiler, TOKEN_COMMAND, "Expected end of command.");
}

/* Delayed resolution: store command tokens for later resolution (dummy chords) */
static bool
compileCommandTokens(Compiler* compiler, Array* cmd, bool inChordArray, const char* message)
{
    assert(compiler), assert(cmd), assert(message);
    if (compiler->panicMode) return false;
    if (inChordArray && check(compiler, TOKEN_RIGHT_PAREN)) return false;

    if (!check(compiler, TOKEN_COMM_INTERP) && !check(compiler, TOKEN_COMMAND))
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
        case TOKEN_USER_VAR:
        case TOKEN_WRAP_CMD_INTERP:
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

/* Delayed mode: compile command or meta command (stores tokens) */
static bool
compileCommandOrMetaDelayed(Compiler* compiler, KeyChord* chord, bool inChordArray, const char* message)
{
    assert(compiler), assert(chord), assert(message);
    if (compiler->panicMode) return false;

    switch (currentType(compiler))
    {
    case TOKEN_GOTO:
    {
        return compileMetaCmdDelayed(compiler, chord);
    }
    default: return compileCommandTokens(compiler, keyChordArray(chord, KC_PROP_COMMAND), inChordArray, message);
    }

    /* UNREACHABLE */
    errorAtCurrent(compiler, "Got unhandled command or meta command...");
    return false;
}

/* Immediate mode: compile command or meta command (resolves to String) */
static bool
compileCommandOrMetaImmediate(Compiler* compiler, KeyChord* chord, size_t index, bool inChordArray, const char* message)
{
    assert(compiler), assert(chord), assert(message);
    if (compiler->panicMode) return false;

    switch (currentType(compiler))
    {
    case TOKEN_GOTO:
    {
        return compileMetaCmdImmediate(compiler, chord, index);
    }
    default: return compileCommand(compiler, chord, index, inChordArray, message);
    }

    /* UNREACHABLE */
    errorAtCurrent(compiler, "Got unhandled command or meta command...");
    return false;
}

/* Helper: compile hook body with immediate resolution (writes to provided String) */
static bool
compileHookBody(Compiler* compiler, KeyChord* chord, String* hook, size_t index, const char* message)
{
    assert(compiler), assert(chord), assert(hook), assert(message);
    if (compiler->panicMode) return false;

    if (!check(compiler, TOKEN_COMM_INTERP) && !check(compiler, TOKEN_COMMAND))
    {
        errorAtCurrent(compiler, message);
        return false;
    }

    const String* desc = keyChordDescriptionConst(chord);

    while (!check(compiler, TOKEN_EOF))
    {
        Token* token = currentToken(compiler);
        switch (token->type)
        {
        case TOKEN_THIS_KEY: stringAppendString(hook, &chord->key.repr); break;
        case TOKEN_INDEX: stringAppendUInt32(compiler->arena, hook, index); break;
        case TOKEN_INDEX_ONE: stringAppendUInt32(compiler->arena, hook, index + 1); break;
        case TOKEN_USER_VAR:
        {
            bool found = false;
            forEach(compiler->userVars, const UserVar, var)
            {
                if (strncmp(var->key, token->start, token->length) == 0)
                {
                    stringAppendCString(hook, var->value);
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                tokenErrorAt(
                    token,
                    compiler->scanner->filepath,
                    "Undefined variable '%%(%.*s)'. Use :var \"%.*s\" \"value\" to define it.",
                    token->length,
                    token->start,
                    token->length,
                    token->start);
                compiler->panicMode = true;
                compiler->hadError  = true;
            }
            break;
        }
        case TOKEN_WRAP_CMD_INTERP:
        {
            if (!stringIsEmpty(&compiler->menu->wrapCmd))
            {
                stringAppendString(hook, &compiler->menu->wrapCmd);
            }
            break;
        }
        case TOKEN_THIS_DESC:
        {
            if (desc && !stringIsEmpty(desc)) stringAppendString(hook, desc);
            break;
        }
        case TOKEN_THIS_DESC_UPPER_FIRST: /* FALLTHROUGH */
        case TOKEN_THIS_DESC_LOWER_FIRST:
        case TOKEN_THIS_DESC_UPPER_ALL:
        case TOKEN_THIS_DESC_LOWER_ALL:
        {
            if (desc && !stringIsEmpty(desc))
            {
                compileDescriptionWithState(compiler, hook, token->type, desc);
            }
            break;
        }
        case TOKEN_COMM_INTERP: /* FALLTHROUGH */
        case TOKEN_COMMAND: stringAppend(hook, token->start, token->length); break;
        default: errorAtCurrent(compiler, "Malformed hook."); return false;
        }
        if (check(compiler, TOKEN_COMMAND)) break;
        advance(compiler);
    }

    stringRtrim(hook);
    return consume(compiler, TOKEN_COMMAND, "Expected end of hook command.");
}

static bool
compileHookImmediate(Compiler* compiler, KeyChord* chord, TokenType type, size_t index)
{
    assert(compiler), assert(chord);
    if (compiler->panicMode) return false;

    /* Note: meta command check removed - for immediate resolution, chord is fully formed */
    switch (type)
    {
    case TOKEN_BEFORE:
    {
        consume(compiler, TOKEN_BEFORE, "Expected '^before' hook.");
        Property* prop = keyChordProperty(chord, KC_PROP_BEFORE);
        PROP_SET_TYPE(prop, STRING);
        String* hook = PROP_VAL(prop, as_string);
        *hook        = stringInit();
        return compileHookBody(compiler, chord, hook, index, "Expected command after '^before' hook.");
    }
    case TOKEN_AFTER:
    {
        consume(compiler, TOKEN_AFTER, "Expected '^after' hook.");
        Property* prop = keyChordProperty(chord, KC_PROP_AFTER);
        PROP_SET_TYPE(prop, STRING);
        String* hook = PROP_VAL(prop, as_string);
        *hook        = stringInit();
        return compileHookBody(compiler, chord, hook, index, "Expected command after '^after' hook.");
    }
    case TOKEN_SYNC_BEFORE:
    {
        consume(compiler, TOKEN_SYNC_BEFORE, "Expected '^sync-before' hook.");
        chord->flags |= FLAG_SYNC_BEFORE;
        Property* prop = keyChordProperty(chord, KC_PROP_BEFORE);
        PROP_SET_TYPE(prop, STRING);
        String* hook = PROP_VAL(prop, as_string);
        *hook        = stringInit();
        return compileHookBody(compiler, chord, hook, index, "Expected command after '^sync-before' hook.");
    }
    case TOKEN_SYNC_AFTER:
    {
        consume(compiler, TOKEN_SYNC_AFTER, "Expected '^sync-after' hook.");
        chord->flags |= FLAG_SYNC_AFTER;
        Property* prop = keyChordProperty(chord, KC_PROP_AFTER);
        PROP_SET_TYPE(prop, STRING);
        String* hook = PROP_VAL(prop, as_string);
        *hook        = stringInit();
        return compileHookBody(compiler, chord, hook, index, "Expected command after '^sync-after' hook.");
    }
    default: return false;
    }
}

/* Delayed resolution: compile hook for dummy chord (stores tokens) */
static bool
compileHookDelayed(Compiler* compiler, KeyChord* chord, TokenType type)
{
    assert(compiler), assert(chord);
    if (compiler->panicMode) return false;

    if (!arrayIsEmpty(keyChordArray(chord, KC_PROP_GOTO)))
    {
        errorAtCurrent(compiler, "Cannot mix hooks and meta commands.");
        return false;
    }

    switch (type)
    {
    case TOKEN_BEFORE:
    {
        consume(compiler, TOKEN_BEFORE, "Expected '^before' hook.");
        return compileCommandTokens(
            compiler,
            keyChordArray(chord, KC_PROP_BEFORE),
            false,
            "Expected command after '^before' hook.");
    }
    case TOKEN_AFTER:
    {
        consume(compiler, TOKEN_AFTER, "Expected '^after' hook.");
        return compileCommandTokens(
            compiler,
            keyChordArray(chord, KC_PROP_AFTER),
            false,
            "Expected command after '^after' hook.");
    }
    case TOKEN_SYNC_BEFORE:
    {
        consume(compiler, TOKEN_SYNC_BEFORE, "Expected '^sync-before' hook.");
        chord->flags |= FLAG_SYNC_BEFORE;
        return compileCommandTokens(
            compiler,
            keyChordArray(chord, KC_PROP_BEFORE),
            false,
            "Expected command after '^sync-before' hook.");
    }
    case TOKEN_SYNC_AFTER:
    {
        consume(compiler, TOKEN_SYNC_AFTER, "Expected '^sync-after' hook.");
        chord->flags |= FLAG_SYNC_AFTER;
        return compileCommandTokens(
            compiler,
            keyChordArray(chord, KC_PROP_AFTER),
            false,
            "Expected command after '^sync-after' hook.");
    }
    default: return false;
    }
}

/* Helper: compile description-like content to a String property (for +title/+wrap in immediate mode) */
static void
compilePropertyString(Compiler* compiler, KeyChord* chord, KeyChordPropId propId, size_t index)
{
    assert(compiler), assert(chord);
    if (compiler->panicMode) return;

    if (!check(compiler, TOKEN_DESC_INTERP) && !check(compiler, TOKEN_DESCRIPTION)) return;

    Property* prop = keyChordProperty(chord, propId);
    PROP_SET_TYPE(prop, STRING);
    String* str = PROP_VAL(prop, as_string);
    *str        = stringInit();

    while (!check(compiler, TOKEN_EOF))
    {
        Token* token = currentToken(compiler);
        switch (token->type)
        {
        case TOKEN_THIS_KEY: stringAppendString(str, &chord->key.repr); break;
        case TOKEN_INDEX: stringAppendUInt32(compiler->arena, str, index); break;
        case TOKEN_INDEX_ONE: stringAppendUInt32(compiler->arena, str, index + 1); break;
        case TOKEN_USER_VAR:
        {
            bool found = false;
            forEach(compiler->userVars, const UserVar, var)
            {
                if (strncmp(var->key, token->start, token->length) == 0)
                {
                    stringAppendCString(str, var->value);
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                tokenErrorAt(
                    token,
                    compiler->scanner->filepath,
                    "Undefined variable '%%(%.*s)'. Use :var \"%.*s\" \"value\" to define it.",
                    token->length,
                    token->start,
                    token->length,
                    token->start);
                compiler->panicMode = true;
                compiler->hadError  = true;
            }
            break;
        }
        case TOKEN_WRAP_CMD_INTERP:
        {
            if (!stringIsEmpty(&compiler->menu->wrapCmd))
            {
                stringAppendString(str, &compiler->menu->wrapCmd);
            }
            break;
        }
        case TOKEN_DESC_INTERP: /* FALLTHROUGH */
        case TOKEN_DESCRIPTION: stringAppendEscString(str, token->start, token->length); break;
        default: errorAtCurrent(compiler, "Malformed property value."); return;
        }
        if (check(compiler, TOKEN_DESCRIPTION)) break;
        advance(compiler);
    }

    consume(compiler, TOKEN_DESCRIPTION, "Expect property value.");
    stringRtrim(str);
}

/* Delayed mode: compile flag (stores tokens for +title/+wrap) */
static bool
compileFlagDelayed(Compiler* compiler, KeyChord* chord, TokenType type)
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
    case TOKEN_ENABLE_SORT: chord->flags &= ~FLAG_IGNORE_SORT; return true;
    case TOKEN_UNWRAP: chord->flags |= FLAG_UNWRAP; return true;
    case TOKEN_TITLE:
    {
        consume(compiler, TOKEN_TITLE, "Expected '+title'.");
        if (check(compiler, TOKEN_DESCRIPTION) || check(compiler, TOKEN_DESC_INTERP))
        {
            compileDescriptionTokens(compiler, keyChordArray(chord, KC_PROP_TITLE));
        }
        return true;
    }
    case TOKEN_WRAP:
    {
        consume(compiler, TOKEN_WRAP, "Expected '+wrap'.");
        if (check(compiler, TOKEN_DESCRIPTION) || check(compiler, TOKEN_DESC_INTERP))
        {
            compileDescriptionTokens(compiler, keyChordArray(chord, KC_PROP_WRAP_CMD));
        }
        return true;
    }
    default: return false;
    }
}

/* Immediate mode: compile flag (resolves +title/+wrap to Strings) */
static bool
compileFlagImmediate(Compiler* compiler, KeyChord* chord, TokenType type, size_t index)
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
    case TOKEN_ENABLE_SORT: chord->flags &= ~FLAG_IGNORE_SORT; return true;
    case TOKEN_UNWRAP: chord->flags |= FLAG_UNWRAP; return true;
    case TOKEN_TITLE:
    {
        consume(compiler, TOKEN_TITLE, "Expected '+title'.");
        compilePropertyString(compiler, chord, KC_PROP_TITLE, index);
        return true;
    }
    case TOKEN_WRAP:
    {
        consume(compiler, TOKEN_WRAP, "Expected '+wrap'.");
        compilePropertyString(compiler, chord, KC_PROP_WRAP_CMD, index);
        return true;
    }
    default: return false;
    }
}

/* Delayed mode: compile hooks and flags, storing tokens for later resolution (dummy chords) */
static void
compileHooksAndFlagsDelayed(Compiler* compiler, KeyChord* chord)
{
    assert(compiler), assert(chord);
    if (compiler->panicMode) return;

    TokenType type = currentType(compiler);
    while (!compilerIsAtEnd(compiler))
    {
        if (!compileHookDelayed(compiler, chord, type) && !compileFlagDelayed(compiler, chord, type))
            return;

        /* Hooks and the `+wrap` flag consume their own token and their argument
         * which leaves them pointing at the next token. */
        if (tokenIsHookType(type) ||
            type == TOKEN_WRAP ||
            type == TOKEN_TITLE)
        {
            type = currentType(compiler);
        }
        else
        {
            type = advance(compiler);
        }
    }
}

/* Immediate mode: compile hooks and flags, resolving interpolations immediately */
static void
compileHooksAndFlagsImmediate(Compiler* compiler, KeyChord* chord, size_t index)
{
    assert(compiler), assert(chord);
    if (compiler->panicMode) return;

    TokenType type = currentType(compiler);
    while (!compilerIsAtEnd(compiler))
    {
        if (!compileHookImmediate(compiler, chord, type, index) &&
            !compileFlagImmediate(compiler, chord, type, index))
            return;

        /* Hooks and the `+wrap` flag consume their own token and their argument
         * which leaves them pointing at the next token. */
        if (tokenIsHookType(type) ||
            type == TOKEN_WRAP ||
            type == TOKEN_TITLE)
        {
            type = currentType(compiler);
        }
        else
        {
            type = advance(compiler);
        }
    }
}

static void
compileMissingKeyChordInfo(Compiler* compiler, const KeyChord* from, KeyChord* to)
{
    assert(compiler), assert(from), assert(to);
    if (compiler->panicMode) return;

    if (!chordFlagHasAnyActive(to->flags)) to->flags = from->flags;

    /* Copy missing properties */
#define COPY_PROP_IF_EMPTY(prop_id)                                \
    do                                                             \
    {                                                              \
        Array*       toProp   = keyChordArray(to, prop_id);        \
        const Array* fromProp = keyChordArrayConst(from, prop_id); \
        if (arrayIsEmpty(toProp) && !arrayIsEmpty(fromProp))       \
        {                                                          \
            *toProp = arrayCopy(fromProp);                         \
        }                                                          \
    } while (0)

    COPY_PROP_IF_EMPTY(KC_PROP_DESCRIPTION);
    COPY_PROP_IF_EMPTY(KC_PROP_COMMAND);
    COPY_PROP_IF_EMPTY(KC_PROP_BEFORE);
    COPY_PROP_IF_EMPTY(KC_PROP_AFTER);
    COPY_PROP_IF_EMPTY(KC_PROP_TITLE);

#undef COPY_PROP_IF_EMPTY
}

static void
compileImplicitChordArray(Compiler* compiler, KeyChord* dummy)
{
    assert(compiler);
    if (compiler->panicMode)
    {
        freeKeyChordArrayProps(dummy);
        return;
    }

    if (!check(compiler, TOKEN_DESCRIPTION) && !check(compiler, TOKEN_DESC_INTERP))
    {
        errorAtCurrent(compiler, "Expected description, or description expressions after '...'.");
        freeKeyChordArrayProps(dummy);
        return;
    }

    compileDescriptionTokens(compiler, keyChordArray(dummy, KC_PROP_DESCRIPTION));

    /* /\* Set FLAG_IGNORE_SORT by default for implicit arrays. */
    /*  * This is done BEFORE compileHooksAndFlagsDelayed so user-provided +sort can override it. *\/ */
    /* dummy->flags |= FLAG_IGNORE_SORT; */

    compileHooksAndFlagsDelayed(compiler, dummy);
    /* TODO think of a better name */
    compileCommandOrMetaDelayed(compiler, dummy, false, "Exepected command.");

    size_t scopeStart = arrayLength(compiler->dest);
    forEach(&compiler->implicitKeys, const Key, key)
    {
        KeyChord chord = { 0 };
        initKeyChordArrayProps(&chord);
        keyCopy(&dummy->key, &chord.key);
        keyCopy(key, &chord.key);
        compileMissingKeyChordInfo(compiler, dummy, &chord);
        /* Resolve Token arrays to Strings using scope-based index */
        resolveChordTokenArrays(compiler, &chord, scopeStart + iter.index);
        arrayAppend(compiler->dest, &chord);
    }

    freeKeyChordArrayProps(dummy);
}

static void
compileChordArray(Compiler* compiler)
{
    assert(compiler);
    if (compiler->panicMode) return;
    if (!isKey(compiler) && !tokenIsModType(currentType(compiler)) &&
        !check(compiler, TOKEN_LEFT_PAREN))
    {
        errorAtCurrent(compiler, "Expect modifier, key, or chord expression after '['.");
        return;
    }

    size_t arrayStart = compilerDestIsEmpty(compiler) ? 0 : getNextIndex(compiler);

    while (!compilerIsAtEnd(compiler) && !check(compiler, TOKEN_RIGHT_BRACKET))
    {
        if (!isKey(compiler) && !tokenIsModType(currentType(compiler)) &&
            !check(compiler, TOKEN_LEFT_PAREN))
        {
            errorAtCurrent(
                compiler,
                "Chord arrays may only contain modifiers, keys, and chord expressions.");
            return;
        }

        KeyChord chord = { 0 };
        initKeyChordArrayProps(&chord);

        if (match(compiler, TOKEN_LEFT_PAREN))
        {
            compileMods(compiler, &chord.key);
            compileKey(compiler, &chord.key);
            compileDescriptionTokens(compiler, keyChordArray(&chord, KC_PROP_DESCRIPTION));
            compileHooksAndFlagsDelayed(compiler, &chord);
            compileCommandOrMetaDelayed(compiler, &chord, true, "Expected command.");
            consume(compiler, TOKEN_RIGHT_PAREN, "Expect closing parenthesis after '('.");
        }
        else
        {
            compileMods(compiler, &chord.key);
            compileKey(compiler, &chord.key);
        }

        appendToDest(compiler, &chord);
    }

    consume(compiler, TOKEN_RIGHT_BRACKET, "Expect ']' after chord array.");
    KeyChord dummy = { 0 };
    initKeyChordArrayProps(&dummy);

    compileDescriptionTokens(compiler, keyChordArray(&dummy, KC_PROP_DESCRIPTION));
    compileHooksAndFlagsDelayed(compiler, &dummy);
    compileCommandOrMetaDelayed(compiler, &dummy, false, "Expected command.");

    /* Write chords in chord array to destination */
    forEachFrom(getDest(compiler), KeyChord, chord, arrayStart)
    {
        compileMissingKeyChordInfo(compiler, &dummy, chord);
        /* Resolve Token arrays to Strings using scope-based index */
        resolveChordTokenArrays(compiler, chord, iter.index);
    }

    freeKeyChordArrayProps(&dummy);
}

static void
compileChord(Compiler* compiler)
{
    assert(compiler);
    if (compiler->panicMode) return;

    /* Check for implicit array first (needs Token arrays for delayed resolution) */
    if (tokenIsModType(currentType(compiler)) || check(compiler, TOKEN_ELLIPSIS))
    {
        KeyChord dummy = { 0 };
        initKeyChordArrayProps(&dummy);
        compileMods(compiler, &dummy.key);
        if (match(compiler, TOKEN_ELLIPSIS)) return compileImplicitChordArray(compiler, &dummy);

        /* Not an ellipsis, proceed as regular chord with immediate resolution */
        KeyChord chord = { 0 };
        keyChordInit(&chord);
        keyCopy(&dummy.key, &chord.key);
        freeKeyChordArrayProps(&dummy);

        compileKey(compiler, &chord.key);
        compileDescription(compiler, &chord, 0);
        compileHooksAndFlagsImmediate(compiler, &chord, 0);

        /* Prefix */
        if (match(compiler, TOKEN_LEFT_BRACE)) return compilePrefix(compiler, &chord);

        compileCommandOrMetaImmediate(compiler, &chord, 0, false, "Expected command.");

        /* Check for brace after command */
        if (check(compiler, TOKEN_LEFT_BRACE))
        {
            errorAtCurrent(compiler, "Expected end of key chord after command but got '{'.");
            return;
        }

        appendToDest(compiler, &chord);
        return;
    }

    /* Regular chord without leading modifiers - use immediate resolution */
    KeyChord chord = { 0 };
    keyChordInit(&chord);

    compileKey(compiler, &chord.key);
    compileDescription(compiler, &chord, 0);
    compileHooksAndFlagsImmediate(compiler, &chord, 0);

    /* Prefix */
    if (match(compiler, TOKEN_LEFT_BRACE)) return compilePrefix(compiler, &chord);

    compileCommandOrMetaImmediate(compiler, &chord, 0, false, "Expected command.");

    /* Check for brace after command */
    if (check(compiler, TOKEN_LEFT_BRACE))
    {
        errorAtCurrent(compiler, "Expected end of key chord after command but got '{'.");
        return;
    }

    appendToDest(compiler, &chord);
}

/* Helper: check if a property has content (works for both Array and String types) */
static bool
propertyHasContent(const Property* prop)
{
    if (!prop) return false;
    switch (prop->type)
    {
    case PROP_TYPE_NONE: return false;
    case PROP_TYPE_ARRAY: return !arrayIsEmpty(&prop->value.as_array);
    case PROP_TYPE_STRING: return !stringIsEmpty(&prop->value.as_string);
    default: return false;
    }
}

/* Helper: copy property from parent to child if child's property is empty */
static void
copyPropertyIfChildEmpty(const KeyChord* parent, KeyChord* child, KeyChordPropId id)
{
    assert(parent), assert(child);

    const Property* parentProp = keyChordPropertyConst(parent, id);
    Property*       childProp  = keyChordProperty(child, id);

    if (!propertyHasContent(parentProp)) return;
    if (propertyHasContent(childProp)) return;

    propertyFree(childProp);
    propertyCopy(parentProp, childProp);
}

static void
setBeforeHook(KeyChord* parent, KeyChord* child)
{
    assert(parent), assert(child);

    /* Children that opt out do not inherit */
    if (chordFlagIsActive(child->flags, FLAG_NO_BEFORE)) return;

    const Property* parentBefore = keyChordPropertyConst(parent, KC_PROP_BEFORE);
    if (!propertyHasContent(parentBefore)) return;

    copyPropertyIfChildEmpty(parent, child, KC_PROP_BEFORE);

    /* set syncBefore flag */
    if (chordFlagIsActive(parent->flags, FLAG_SYNC_BEFORE)) child->flags |= FLAG_SYNC_BEFORE;
}

static void
setAfterHook(KeyChord* parent, KeyChord* child)
{
    assert(parent), assert(child);

    /* Children that opt out do not inherit */
    if (chordFlagIsActive(child->flags, FLAG_NO_AFTER)) return;

    const Property* parentAfter = keyChordPropertyConst(parent, KC_PROP_AFTER);
    if (!propertyHasContent(parentAfter)) return;

    copyPropertyIfChildEmpty(parent, child, KC_PROP_AFTER);

    /* set syncAfter flag */
    if (chordFlagIsActive(parent->flags, FLAG_SYNC_AFTER)) child->flags |= FLAG_SYNC_AFTER;
}

static void
setHooks(KeyChord* parent, KeyChord* child)
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
setHooksAndFlags(KeyChord* parent, Array* children)
{
    assert(parent), assert(children);

    forEach(children, KeyChord, child)
    {
        if (chordFlagIsActive(child->flags, FLAG_IGNORE)) continue;
        if (!arrayIsEmpty(&child->keyChords) && !chordFlagIsActive(child->flags, FLAG_INHERIT))
            continue;

        setHooks(parent, child);
        child->flags = setFlags(parent->flags, child->flags);

        /* Inherit wrapper command if child doesn't unwrap and doesn't have its own */
        const Property* parentWrapCmd = keyChordPropertyConst(parent, KC_PROP_WRAP_CMD);
        const Property* childWrapCmd  = keyChordPropertyConst(child, KC_PROP_WRAP_CMD);
        if (!chordFlagIsActive(child->flags, FLAG_UNWRAP) &&
            !propertyHasContent(childWrapCmd) &&
            propertyHasContent(parentWrapCmd))
        {
            copyPropertyIfChildEmpty(parent, child, KC_PROP_WRAP_CMD);
        }

        /* Inherit title if:
         * - child is a prefix
         * - child does not have a title
         * - parent has a title */
        const Property* childTitle  = keyChordPropertyConst(child, KC_PROP_TITLE);
        const Property* parentTitle = keyChordPropertyConst(parent, KC_PROP_TITLE);
        if (!arrayIsEmpty(&child->keyChords) &&
            !propertyHasContent(childTitle) &&
            propertyHasContent(parentTitle))
        {
            copyPropertyIfChildEmpty(parent, child, KC_PROP_TITLE);
        }

        if (!arrayIsEmpty(&child->keyChords))
        {
            setHooksAndFlags(child, &child->keyChords);
        }
    }
}

static void
compilePrefix(Compiler* compiler, KeyChord* chord)
{
    assert(compiler), assert(chord);
    if (compiler->panicMode)
    {
        keyChordFree(chord);
        return;
    }

    const Property* gotoProp = keyChordPropertyConst(chord, KC_PROP_GOTO);
    if (propertyHasContent(gotoProp))
    {
        errorAtCurrent(compiler, "Cannot mix @goto and prefixes.");
        keyChordFree(chord);
        return;
    }

    /* Backup information */
    Array* previousDest = getDest(compiler);

    /* advance */
    appendToDest(compiler, chord);
    KeyChord* parent   = ARRAY_GET_LAST(previousDest, KeyChord);
    Array*    children = &parent->keyChords;
    compiler->dest     = children;

    /* Compile children */
    while (!compilerIsAtEnd(compiler) && !check(compiler, TOKEN_RIGHT_BRACE))
    {
        compileKeyChord(compiler);
    }
    consume(compiler, TOKEN_RIGHT_BRACE, "Expect '}' after prefix.");

    if (arrayIsEmpty(children))
    {
        errorAt(compiler, previousToken(compiler), "No key chords set for prefix.");
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
        case TOKEN_LEFT_BRACKET:
            return; /* No need to skip past. */
        /* Seek to modifier or key and return. */
        case TOKEN_COMM_INTERP: /* FALLTHROUGH */
        case TOKEN_COMMAND:
        {
            /* Will consume until TOKEN_COMMAND */
            while (!compilerIsAtEnd(compiler) && !check(compiler, TOKEN_COMMAND))
            {
                advance(compiler);
            }
            if (compilerIsAtEnd(compiler)) return;
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
        KeyChord dummy = { 0 };
        initKeyChordArrayProps(&dummy);
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
    case TOKEN_THIS_KEY: stringAppendString(dest, &to->key.repr); break;
    case TOKEN_THIS_DESC:
    {
        const String* desc = keyChordDescriptionConst(to);
        if (!stringIsEmpty(desc)) stringAppendString(dest, desc);
        break;
    }
    case TOKEN_THIS_DESC_UPPER_FIRST: /* FALLTHROUGH */
    case TOKEN_THIS_DESC_LOWER_FIRST:
    case TOKEN_THIS_DESC_UPPER_ALL:
    case TOKEN_THIS_DESC_LOWER_ALL:
    {
        const String* desc = keyChordDescriptionConst(to);
        if (!stringIsEmpty(desc))
        {
            compileDescriptionWithState(compiler, dest, token->type, desc);
        }
        break;
    }
    case TOKEN_INDEX: stringAppendUInt32(compiler->arena, dest, index); break;
    case TOKEN_INDEX_ONE: stringAppendUInt32(compiler->arena, dest, index + 1); break;
    case TOKEN_WRAP_CMD_INTERP:
    {
        if (!stringIsEmpty(&compiler->menu->wrapCmd))
        {
            stringAppendString(dest, &compiler->menu->wrapCmd);
        }
        break;
    }
    case TOKEN_DESC_INTERP: /* FALLTHROUGH */
    case TOKEN_DESCRIPTION: stringAppendEscString(dest, token->start, token->length); break;
    case TOKEN_COMM_INTERP: /* FALLTHROUGH */
    case TOKEN_COMMAND: stringAppend(dest, token->start, token->length); break;
    case TOKEN_USER_VAR:
    {
        bool found = false;
        forEach(compiler->userVars, const UserVar, var)
        {
            if (strncmp(var->key, token->start, token->length) == 0)
            {
                stringAppendCString(dest, var->value);
                found = true;
                break;
            }
        }

        if (!found)
        {
            tokenErrorAt(
                token,
                compiler->scanner->filepath,
                "Undefined variable '%%(%.*s)'. Use :var \"%.*s\" \"value\" to define it.",
                token->length,
                token->start,
                token->length,
                token->start);
            compiler->panicMode = true;
            compiler->hadError  = true;
        }

        break;
    }
    default:
    {
        errorMsg(
            "Got unexpected token when compiling token array: '%s'.",
            tokenGetLiteral(token->type));
        break;
    }
    }
}

static void
compileStringFromTokens(Compiler* compiler, Array* tokens, KeyChord* to, String* dest, size_t index)
{
    assert(compiler), assert(tokens), assert(to), assert(dest);

    forEach(tokens, Token, token)
    {
        compileStringFromToken(compiler, token, to, dest, index);
    }

    stringRtrim(dest);
}

/* Resolve a single chord's Token array properties to Strings with given source index */
static void
resolveChordTokenArrays(Compiler* compiler, KeyChord* chord, size_t sourceIndex)
{
    assert(compiler), assert(chord);

    for (size_t i = 0; i < KC_PROP_COUNT; i++)
    {
        Property* prop = keyChordProperty(chord, (KeyChordPropId)i);
        if (prop->type == PROP_TYPE_ARRAY)
        {
            Array* tokens = PROP_VAL(prop, as_array);
            if (!arrayIsEmpty(tokens))
            {
                String result = stringInit();
                compileStringFromTokens(compiler, tokens, chord, &result, sourceIndex);
                arrayFree(tokens);
                prop->type            = PROP_TYPE_STRING;
                prop->value.as_string = result;
            }
            else
            {
                /* Empty array - just mark as unset */
                arrayFree(tokens);
                prop->type = PROP_TYPE_NONE;
            }
        }
    }
}

/* Resolve Token arrays in KeyChord properties to final Strings */
static void
resolveKeyChordProperties(Compiler* compiler, Array* keyChords)
{
    assert(compiler), assert(keyChords);

    forEach(keyChords, KeyChord, kc)
    {
        /* Resolve each property that has PROP_TYPE_ARRAY */
        for (size_t i = 0; i < KC_PROP_COUNT; i++)
        {
            Property* prop = keyChordProperty(kc, (KeyChordPropId)i);
            if (prop->type == PROP_TYPE_ARRAY)
            {
                Array* tokens = PROP_VAL(prop, as_array);
                if (!arrayIsEmpty(tokens))
                {
                    String result = stringInit();
                    compileStringFromTokens(compiler, tokens, kc, &result, iter.index);
                    arrayFree(tokens);
                    prop->type            = PROP_TYPE_STRING;
                    prop->value.as_string = result;
                }
                else
                {
                    /* Empty array - just mark as unset */
                    arrayFree(tokens);
                    prop->type = PROP_TYPE_NONE;
                }
            }
        }

        /* Recursively resolve nested key chords */
        if (!arrayIsEmpty(&kc->keyChords))
        {
            resolveKeyChordProperties(compiler, &kc->keyChords);
        }
    }
}

static bool
compileImplicitChordArrayKeys(Compiler* compiler, Menu* menu)
{
    assert(compiler), assert(menu);

    scannerInit(&compiler->implicitArrayKeysScanner, menu->implicitArrayKeys, ".");
    compiler->scanner = &compiler->implicitArrayKeysScanner;

    TokenType type = advance(compiler);
    while (type != TOKEN_EOF)
    {
        switch (type)
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
        type = currentType(compiler);
    }

    tokenInit(currentToken(compiler));
    compiler->scanner = &compiler->sourceScanner;
    return true;
}

static void
compilerFree(Compiler* compiler)
{
    assert(compiler);

    if (compiler->chords != NULL) freeKeyChordArrayPropsRecursive(compiler->chords);
    forEach(&compiler->implicitKeys, Key, key) { keyFree(key); }
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

    Array chords     = ARRAY_INIT(KeyChord);
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

    deduplicateKeyChordArray(compiler->chords);
    if (compiler->sort) keyChordArraySort(compiler->chords);

    /* Resolve Token arrays to Strings */
    resolveKeyChordProperties(compiler, compiler->chords);

    /* Transfer ownership to menu */
    menu->compiledKeyChords = *compiler->chords;
    menu->keyChords         = &menu->compiledKeyChords;
    compiler->chords        = NULL; /* Prevent double-free in compilerFree */

    if (compiler->debug)
    {
        debugPrintScannedTokenFooter();
        disassembleKeyChordArray(menu->keyChords, 0);
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
    compiler->implicitKeys = ARRAY_INIT(Key);
    compiler->dest         = NULL;
    compiler->chords       = NULL;
    compiler->arena        = &menu->arena;
    compiler->userVars     = &menu->userVars;
    compiler->menu         = menu;
    compiler->delimiter    = menu->delimiter;
    compiler->source       = source;
    compiler->delimiterLen = strlen(menu->delimiter);
    compiler->hadError     = false;
    compiler->panicMode    = false;
    compiler->sort         = menu->sort;
    compiler->debug        = menu->debug;
}
