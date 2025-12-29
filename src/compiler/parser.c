#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* common includes */
#include "common/key_chord.h"
#include "common/menu.h"
#include "common/property.h"
#include "common/span.h"
#include "common/string.h"
#include "common/vector.h"

/* local includes */
#include "args.h"
#include "compiler.h"
#include "debug.h"
#include "expect.h"
#include "handler.h"
#include "parser.h"
#include "scanner.h"
#include "token.h"
#include "transform.h"

typedef struct
{
    const char* start;
    size_t      length;
} InternedIndex;

static InternedIndex indexInterns[256] = { 0 };

/* Maximum prefix nesting depth */
#define MAX_DEPTH 32

struct Parser
{
    Scanner*    scanner;
    Token       current;
    Token       previous;
    KeyChord*   chord;
    Vector*     dest;
    Vector      rootChords;
    KeyChord*   parentStack[MAX_DEPTH];
    Vector*     destStack[MAX_DEPTH];
    Vector      childArrays[MAX_DEPTH];
    size_t      depth;
    Expectation expect;
    Arena*      arena;
    Vector*     userVars;
    Vector      implicitKeys;
    Menu*       menu;
    Stack       argEnvStack;
    bool        chordPushedEnv;
    bool        pushedEnvAtDepth[MAX_DEPTH];
    bool        inTemplateContext;
    bool        hadError;
    bool        panicMode;
    bool        debug;
};

static Token
indexToToken(Arena* arena, size_t index, const Token* token, TokenType interpType)
{
    assert(arena), assert(token);

    size_t value = (token->type == TOKEN_INDEX_ONE) ? index + 1 : index;

    if (value >= 256)
    {
        return *token;
    }

    if (!indexInterns[value].start)
    {
        char buf[12];
        int  len                   = snprintf(buf, sizeof(buf), "%zu", value);
        indexInterns[value].start  = arenaCopyCString(arena, buf, len);
        indexInterns[value].length = len;
    }

    Token result  = *token;
    result.type   = interpType;
    result.start  = indexInterns[value].start;
    result.length = indexInterns[value].length;
    return result;
}

static TokenType
getInterpTypeForProperty(PropId id)
{
    switch (id)
    {
    case KC_PROP_COMMAND: /* FALLTHROUGH */
    case KC_PROP_BEFORE:
    case KC_PROP_AFTER:
    case KC_PROP_WRAP_CMD: return TOKEN_COMM_INTERP;
    default: return TOKEN_DESC_INTERP;
    }
}

static const Vector*
lookupArgTokens(const ArgEnvironment* tupleEnv, const Stack* parentEnvStack, size_t argIndex)
{
    const Vector* arg = NULL;

    if (tupleEnv)
    {
        arg = argEnvGetConst(tupleEnv, argIndex);
    }
    if (!arg && parentEnvStack)
    {
        arg = argEnvStackLookupConst(parentEnvStack, argIndex);
    }

    return arg;
}

static void
appendArgTokensWithConversion(
    Arena*        arena,
    const Vector* arg,
    size_t        index,
    TokenType     interpType,
    Vector*       dest)
{
    assert(arena), assert(arg), assert(dest);

    vectorForEach(arg, const Token, argToken)
    {
        Token converted = *argToken;

        if (argToken->type == TOKEN_DESC_INTERP && interpType == TOKEN_COMM_INTERP)
        {
            converted.type = TOKEN_COMM_INTERP;
        }

        if (converted.type == TOKEN_INDEX || converted.type == TOKEN_INDEX_ONE)
        {
            converted = indexToToken(arena, index, &converted, interpType);
        }

        vectorAppend(dest, &converted);
    }
}

static void
copyPropertyArray(
    Arena*                arena,
    const Stack*          parentEnvStack,
    PropId                propId,
    const Property*       fromProp,
    Property*             toProp,
    size_t                index,
    const ArgEnvironment* tupleEnv)
{
    assert(arena), assert(fromProp), assert(toProp);

    PROP_SET_TYPE(toProp, ARRAY);
    toProp->value.as_array = VECTOR_INIT(Token);

    const Vector* fromVec    = &fromProp->value.as_array;
    Vector*       toVec      = &toProp->value.as_array;
    TokenType     interpType = getInterpTypeForProperty(propId);

    vectorForEach(fromVec, const Token, token)
    {
        if (token->type == TOKEN_ARG_POSITION)
        {
            size_t        argIndex = strtoul(token->start, NULL, 10);
            const Vector* arg      = lookupArgTokens(tupleEnv, parentEnvStack, argIndex);

            if (arg)
            {
                appendArgTokensWithConversion(arena, arg, index, interpType, toVec);
            }
            else
            {
                vectorAppend(toVec, token);
            }
        }
        else if (token->type == TOKEN_INDEX || token->type == TOKEN_INDEX_ONE)
        {
            Token resolved = indexToToken(arena, index, token, interpType);
            vectorAppend(toVec, &resolved);
        }
        else
        {
            vectorAppend(toVec, token);
        }
    }
}

static void
applySharedPropertiesToChord(
    Parser*               p,
    const KeyChord*       from,
    KeyChord*             to,
    size_t                index,
    const ArgEnvironment* tupleEnv)
{
    assert(p), assert(from), assert(to);

    if (to->flags == FLAG_NONE) to->flags = from->flags;

    Arena* arena = parserArena(p);
    for (PropId id = 0; id < KC_PROP_COUNT; id++)
    {
        const Property* fromProp = propGetConst(from, id);
        Property*       toProp   = propGet(to, id);

        if (propertyHasContent(fromProp) && !propertyHasContent(toProp) &&
            fromProp->type == PROP_TYPE_ARRAY)
        {
            copyPropertyArray(arena, parserArgEnvStack(p), id, fromProp, toProp, index, tupleEnv);
        }
    }
}

static void
applySharedProperties(
    Parser*         p,
    Vector*         partials,
    Vector*         tupleEnvs,
    const KeyChord* tmpl)
{
    assert(p), assert(partials), assert(tupleEnvs), assert(tmpl);

    Vector* dest = parserDest(p);

    vectorForEach(partials, KeyChord, partial)
    {
        ArgEnvironment* tupleEnv = VECTOR_GET(tupleEnvs, ArgEnvironment, iter.index);
        applySharedPropertiesToChord(p, tmpl, partial, iter.index, tupleEnv);

        KeyChord* slot = VECTOR_APPEND_SLOT(dest, KeyChord);
        *slot          = *partial;
    }
}

static bool
parseKeyIntoChord(Parser* p, KeyChord* chord)
{
    assert(p), assert(chord);

    Token* token = parserCurrentToken(p);

    while (tokenIsModType(token->type))
    {
        switch (token->type)
        {
        case TOKEN_MOD_CTRL: chord->key.mods |= MOD_CTRL; break;
        case TOKEN_MOD_META: chord->key.mods |= MOD_META; break;
        case TOKEN_MOD_HYPER: chord->key.mods |= MOD_HYPER; break;
        case TOKEN_MOD_SHIFT: chord->key.mods |= MOD_SHIFT; break;
        default: break;
        }
        parserAdvance(p);
        token = parserCurrentToken(p);
    }

    Arena* arena = parserArena(p);
    if (token->type == TOKEN_SPECIAL_KEY)
    {
        chord->key.special = token->special;
        chord->key.repr    = stringFromCString(arena, specialKeyRepr(chord->key.special));
        parserAdvance(p);
        return true;
    }
    else if (token->type == TOKEN_KEY)
    {
        chord->key.repr = stringMake(arena, token->start, token->length);
        parserAdvance(p);
        return true;
    }

    return false;
}

static bool
parseChordContent(
    Parser*     p,
    KeyChord*   chord,
    Expectation terminator,
    Expectation initialExpect)
{
    assert(p), assert(chord);

    KeyChord* saved = parserCurrentChord(p);
    parserSetChord(p, chord);

    Expectation expect = initialExpect;

    while (!parserIsAtEnd(p))
    {
        Expectation got = tokenToExpectation(parserCurrentToken(p)->type);

        if (got & terminator) break;

        if (got == EXPECT_COMMAND || got == EXPECT_META)
        {
            HandleResult result = handleCurrent(p);
            if (result.error)
            {
                parserSetChord(p, saved);
                return false;
            }
            break;
        }

        if (!(got & expect))
        {
            parserErrorUnexpected(p, expect, got);
            parserSetChord(p, saved);
            return false;
        }

        HandleResult result = handleCurrent(p);
        if (result.error)
        {
            parserSetChord(p, saved);
            return false;
        }

        expect = result.next;
    }

    parserSetChord(p, saved);
    return true;
}

static bool
parseTuple(Parser* p, KeyChord* tuple)
{
    assert(p), assert(tuple);

    compilerInitChord(tuple);

    if (!parserCheck(p, TOKEN_LEFT_PAREN))
    {
        parserErrorAtCurrent(p, "Expected '(' to start tuple.");
        return false;
    }
    parserAdvance(p);

    if (!parseKeyIntoChord(p, tuple))
    {
        parserErrorAtCurrent(p, "Expected key in tuple.");
        return false;
    }

    if (!parseChordContent(p, tuple, EXPECT_RPAREN, EXPECT_DESC))
    {
        return false;
    }

    if (!parserCheck(p, TOKEN_RIGHT_PAREN))
    {
        parserErrorAtCurrent(p, "Expected ')' to end tuple.");
        return false;
    }
    parserAdvance(p);

    return true;
}

static void
parserFree(Parser* p)
{
    assert(p);

    vectorForEach(&p->implicitKeys, Key, key) { keyFree(key); }
    vectorFree(&p->implicitKeys);

    for (size_t i = 0; i < MAX_DEPTH; i++)
    {
        vectorFree(&p->childArrays[i]);
    }

    while (!stackIsEmpty(&p->argEnvStack))
    {
        ArgEnvironment* env = STACK_PEEK(&p->argEnvStack, ArgEnvironment);
        argEnvFree(env);
        stackPop(&p->argEnvStack);
    }
    stackFree(&p->argEnvStack);
}

static void
parserInit(Parser* p, Scanner* scanner, Menu* m)
{
    assert(p), assert(scanner), assert(m);

    p->scanner      = scanner;
    p->rootChords   = VECTOR_INIT(KeyChord);
    p->dest         = &p->rootChords;
    p->chord        = NULL;
    p->depth        = 0;
    p->expect       = EXPECT_KEY_START | EXPECT_EOF;
    p->arena        = &m->arena;
    p->userVars     = &m->userVars;
    p->implicitKeys = VECTOR_INIT(Key);
    for (size_t i = 0; i < MAX_DEPTH; i++)
    {
        p->childArrays[i] = VECTOR_INIT(KeyChord);
    }
    p->argEnvStack       = STACK_INIT(ArgEnvironment);
    p->chordPushedEnv    = false;
    p->inTemplateContext = false;
    memset(p->pushedEnvAtDepth, 0, sizeof(p->pushedEnvAtDepth));
    p->menu      = m;
    p->hadError  = false;
    p->panicMode = false;
    p->debug     = m->debug;
    tokenInit(&p->current);
    tokenInit(&p->previous);
}

static bool
parseImplicitKeysImpl(Parser* p, const char* keysString)
{
    assert(p);

    bool result = true;

    if (!keysString || *keysString == '\0') return result;

    Scanner scanner;
    Parser  keyParser;
    Vector* implicitKeys = &p->implicitKeys;
    scannerInit(&scanner, keysString, "KEYS");
    parserInit(&keyParser, &scanner, p->menu);

    Expectation keyExpectation = EXPECT_MOD | EXPECT_KEY | EXPECT_EOF;

    parserAllocChord(&keyParser);
    parserAdvance(&keyParser);

    while (!parserIsAtEnd(&keyParser))
    {
        Expectation got = tokenToExpectation(keyParser.current.type);

        if (!(got & keyExpectation))
        {
            parserErrorUnexpected(&keyParser, keyExpectation, got);
            result = false;
            goto end;
        }

        HandleResult handleResult = handleCurrent(&keyParser);

        if (handleResult.error)
        {
            result = false;
            goto end;
        }

        if (got & EXPECT_KEY)
        {
            KeyChord* chord = parserCurrentChord(&keyParser);
            vectorAppend(implicitKeys, &chord->key);
            keyInit(&chord->key);
        }
    }

end:
    compilerFreeChordVector(&keyParser.rootChords);
    return result;
}

Token*
parserCurrentToken(Parser* p)
{
    assert(p);
    return &p->current;
}

Token*
parserPreviousToken(Parser* p)
{
    assert(p);
    return &p->previous;
}

bool
parserCheck(Parser* p, TokenType type)
{
    assert(p);
    return p->current.type == type;
}

bool
parserIsAtEnd(Parser* p)
{
    assert(p);
    return p->current.type == TOKEN_EOF;
}

KeyChord*
parserCurrentChord(Parser* p)
{
    assert(p);
    return p->chord;
}

void
parserSetChord(Parser* p, KeyChord* chord)
{
    assert(p);
    p->chord = chord;
}

Vector*
parserDest(Parser* p)
{
    assert(p);
    return p->dest;
}

void
parserSetDest(Parser* p, Vector* dest)
{
    assert(p);
    p->dest = dest;
}

Arena*
parserArena(Parser* p)
{
    assert(p);
    return p->arena;
}

Vector*
parserUserVars(Parser* p)
{
    assert(p);
    return p->userVars;
}

Vector*
parserImplicitKeys(Parser* p)
{
    assert(p);
    return &p->implicitKeys;
}

Menu*
parserMenu(Parser* p)
{
    assert(p);
    return p->menu;
}

Scanner*
parserScanner(Parser* p)
{
    assert(p);
    return p->scanner;
}

bool
parserHadError(Parser* p)
{
    assert(p);
    return p->hadError;
}

void
parserSetError(Parser* p)
{
    assert(p);
    p->hadError = true;
}

bool
parserInPanicMode(Parser* p)
{
    assert(p);
    return p->panicMode;
}

void
parserSetPanicMode(Parser* p, bool mode)
{
    assert(p);
    p->panicMode = mode;
}

bool
parserDebug(Parser* p)
{
    return p->debug;
}

size_t
parserDepth(Parser* p)
{
    assert(p);
    return p->depth;
}

Vector*
parserSavedDest(Parser* p, size_t depth)
{
    assert(p);
    assert(depth < MAX_DEPTH);
    return p->destStack[depth];
}

KeyChord*
parserSavedChord(Parser* p, size_t depth)
{
    assert(p);
    assert(depth < MAX_DEPTH);
    return p->parentStack[depth];
}

Vector*
parserChildVector(Parser* p, size_t depth)
{
    assert(p), assert(depth < MAX_DEPTH);

    Vector* vec = &p->childArrays[depth];
    if (vec->elementSize == 0)
    {
        *vec = VECTOR_INIT(KeyChord);
    }
    else
    {
        vec->length = 0;
    }
    return vec;
}

TokenType
parserAdvance(Parser* p)
{
    assert(p);

    tokenCopy(&p->current, &p->previous);

    while (true)
    {
        scannerTokenForCompiler(p->scanner, &p->current);
        if (p->debug) disassembleToken(&p->current);
        if (p->current.type != TOKEN_ERROR) break;

        parserErrorAtCurrent(p, "%s", p->current.message);
    }

    return p->current.type;
}

KeyChord*
parserAllocChord(Parser* p)
{
    assert(p);

    KeyChord* chord = VECTOR_APPEND_SLOT(p->dest, KeyChord);
    compilerInitChord(chord);
    p->chord          = chord;
    p->chordPushedEnv = false;

    return chord;
}

static void
popArgEnvAndWarn(Parser* p)
{
    ArgEnvironment* top = STACK_PEEK(&p->argEnvStack, ArgEnvironment);
    if (top)
    {
        argEnvWarnUnused(top, p->scanner);
        ArgEnvironment env = *top;
        stackPop(&p->argEnvStack);
        argEnvFree(&env);
    }
}

void
parserFinishChord(Parser* p)
{
    assert(p);

    if (p->chordPushedEnv)
    {
        popArgEnvAndWarn(p);
        p->chordPushedEnv = false;
    }

    parserAllocChord(p);
}

void
parserPushState(Parser* p)
{
    assert(p);
    assert(p->depth < MAX_DEPTH);

    p->parentStack[p->depth] = p->chord;
    p->destStack[p->depth]   = p->dest;
    p->depth++;
}

bool
parserPopState(Parser* p)
{
    assert(p);

    if (p->depth == 0) return false;

    p->depth--;
    return true;
}

Stack*
parserArgEnvStack(Parser* p)
{
    assert(p);
    return &p->argEnvStack;
}

bool
parserChordPushedEnv(Parser* p)
{
    assert(p);
    return p->chordPushedEnv;
}

void
parserSetChordPushedEnv(Parser* p, bool pushed)
{
    assert(p);
    p->chordPushedEnv = pushed;
}

bool
parserInTemplateContext(Parser* p)
{
    assert(p);
    return p->inTemplateContext;
}

void
parserSetInTemplateContext(Parser* p, bool inTemplate)
{
    assert(p);
    p->inTemplateContext = inTemplate;
}

bool
parserPushedEnvAtDepth(Parser* p, size_t depth)
{
    assert(p);
    assert(depth < MAX_DEPTH);
    return p->pushedEnvAtDepth[depth];
}

void
parserSetPushedEnvAtDepth(Parser* p, size_t depth, bool pushed)
{
    assert(p);
    assert(depth < MAX_DEPTH);
    p->pushedEnvAtDepth[depth] = pushed;
}

Expectation
parserNextChordExpectation(Parser* p)
{
    assert(p);

    if (p->depth > 0)
    {
        return EXPECT_KEY_START | EXPECT_RBRACE;
    }
    return EXPECT_KEY_START | EXPECT_EOF;
}

void
parserDebugAt(Parser* p, Token* token, const char* fmt, ...)
{
    assert(p), assert(token), assert(fmt);

    if (!p->debug) return;
    if (p->panicMode) return;

    va_list ap;
    va_start(ap, fmt);
    vscannerDebugAt(p->scanner, token, fmt, ap);
    va_end(ap);
}

void
parserErrorAt(Parser* p, Token* token, const char* fmt, ...)
{
    assert(p), assert(token), assert(fmt);

    if (p->panicMode) return;

    p->panicMode = true;
    p->hadError  = true;

    va_list ap;
    va_start(ap, fmt);
    vscannerErrorAt(p->scanner, token, fmt, ap);
    va_end(ap);
}

void
parserWarnAt(Parser* p, Token* token, const char* fmt, ...)
{
    assert(p), assert(token), assert(fmt);

    if (p->panicMode) return;

    va_list ap;
    va_start(ap, fmt);
    vscannerWarnAt(p->scanner, token, fmt, ap);
    va_end(ap);
}

void
parserErrorAtCurrent(Parser* p, const char* fmt, ...)
{
    assert(p), assert(fmt);

    if (p->panicMode) return;

    p->panicMode = true;
    p->hadError  = true;

    va_list ap;
    va_start(ap, fmt);
    vscannerErrorAt(p->scanner, &p->current, fmt, ap);
    va_end(ap);
}

void
parserErrorUnexpected(Parser* p, Expectation expected, Expectation got)
{
    assert(p);

    char expectedBuf[256];
    char gotBuf[256];

    parserErrorAtCurrent(
        p,
        "Expected %s but got %s.",
        expectationToString(expected, expectedBuf, sizeof(expectedBuf)),
        expectationToString(got, gotBuf, sizeof(gotBuf)));
}

static KeyChord
parseSharedTemplate(Parser* p)
{
    assert(p);

    KeyChord tmpl;
    compilerInitChord(&tmpl);

    parserSetInTemplateContext(p, true);

    if (!parseChordContent(p, &tmpl, EXPECT_KEY_START | EXPECT_RBRACE | EXPECT_EOF, EXPECT_DESC))
    {
        compilerFreeChord(&tmpl);
        parserSetInTemplateContext(p, false);
        compilerInitChord(&tmpl);
        return tmpl;
    }

    parserSetInTemplateContext(p, false);
    return tmpl;
}

static bool
parseRightBrace(Parser* p)
{
    assert(p);

    if (parserDepth(p) == 0)
    {
        parserErrorAtCurrent(p, "Unexpected '}'.");
        return false;
    }

    Vector* dest = parserDest(p);

    if (vectorIsEmpty(dest))
    {
        parserErrorAtCurrent(p, "Empty prefix.");
        return false;
    }

    KeyChord* lastChord = parserCurrentChord(p);
    if (lastChord && stringIsEmpty(&lastChord->key.repr))
    {
        compilerFreeChord(lastChord);
        vectorRemove(dest, vectorLength(dest) - 1);
    }

    deduplicateVector(dest, compilerFreeChord);

    KeyChord* parentChord = parserSavedChord(p, parserDepth(p) - 1);

    parentChord->keyChords = SPAN_FROM_VECTOR(parserArena(p), dest, KeyChord);

    if (!parserPopState(p))
    {
        parserErrorAtCurrent(p, "State stack underflow.");
        return false;
    }

    size_t depth = parserDepth(p);
    if (p->pushedEnvAtDepth[depth])
    {
        popArgEnvAndWarn(p);
        p->pushedEnvAtDepth[depth] = false;
    }

    parserSetDest(p, parserSavedDest(p, depth));
    parserAdvance(p);
    parserAllocChord(p);

    p->expect = parserNextChordExpectation(p);
    return true;
}

static void
appendEmptyEnv(Vector* tupleEnvs)
{
    assert(tupleEnvs);

    ArgEnvironment emptyEnv;
    argEnvInit(&emptyEnv);
    vectorAppend(tupleEnvs, &emptyEnv);
}

static void
accumulateModifier(Modifier* mods, TokenType type)
{
    assert(mods);

    switch (type)
    {
    case TOKEN_MOD_CTRL: *mods |= MOD_CTRL; break;
    case TOKEN_MOD_META: *mods |= MOD_META; break;
    case TOKEN_MOD_HYPER: *mods |= MOD_HYPER; break;
    case TOKEN_MOD_SHIFT: *mods |= MOD_SHIFT; break;
    default: break;
    }
}

static void
finalizePartialKey(
    Parser*   p,
    Token*    token,
    KeyChord* currentPartial,
    Modifier  modPrefix,
    Vector*   partialChords,
    Vector*   tupleEnvs)
{
    assert(p), assert(token), assert(currentPartial), assert(partialChords), assert(tupleEnvs);

    Arena* arena = parserArena(p);
    if (token->type == TOKEN_SPECIAL_KEY)
    {
        currentPartial->key.special = token->special;
        currentPartial->key.repr    = stringFromCString(arena, specialKeyRepr(currentPartial->key.special));
    }
    else
    {
        currentPartial->key.repr = stringMake(arena, token->start, token->length);
    }

    KeyChord* slot = VECTOR_APPEND_SLOT(partialChords, KeyChord);
    *slot          = *currentPartial;
    appendEmptyEnv(tupleEnvs);

    compilerInitChord(currentPartial);
    currentPartial->key.mods = modPrefix;
}

static bool
commitPendingPartial(
    KeyChord* currentPartial,
    Modifier  modPrefix,
    Vector*   partialChords,
    Vector*   tupleEnvs)
{
    assert(currentPartial), assert(partialChords), assert(tupleEnvs);

    if (!stringIsEmpty(&currentPartial->key.repr))
    {
        KeyChord* slot = VECTOR_APPEND_SLOT(partialChords, KeyChord);
        *slot          = *currentPartial;
        appendEmptyEnv(tupleEnvs);
        compilerInitChord(currentPartial);
        currentPartial->key.mods = modPrefix;
        return true;
    }
    return false;
}

static ArgEnvironment
popTupleEnv(Parser* p)
{
    assert(p);

    ArgEnvironment tupleEnv;
    if (parserChordPushedEnv(p))
    {
        ArgEnvironment* top = STACK_PEEK(parserArgEnvStack(p), ArgEnvironment);
        if (top)
        {
            tupleEnv = *top;
            stackPop(parserArgEnvStack(p));
        }
        else
        {
            argEnvInit(&tupleEnv);
        }
        parserSetChordPushedEnv(p, false);
    }
    else
    {
        argEnvInit(&tupleEnv);
    }
    return tupleEnv;
}

static bool
parseArrayTuple(
    Parser*   p,
    KeyChord* currentPartial,
    Modifier  modPrefix,
    Vector*   partialChords,
    Vector*   tupleEnvs)
{
    assert(p), assert(currentPartial), assert(partialChords), assert(tupleEnvs);

    commitPendingPartial(currentPartial, modPrefix, partialChords, tupleEnvs);

    KeyChord tuple;
    if (!parseTuple(p, &tuple))
    {
        if (parserChordPushedEnv(p))
        {
            ArgEnvironment* top = STACK_PEEK(parserArgEnvStack(p), ArgEnvironment);
            if (top)
            {
                argEnvFree(top);
                stackPop(parserArgEnvStack(p));
            }
            parserSetChordPushedEnv(p, false);
        }
        return false;
    }

    ArgEnvironment tupleEnv = popTupleEnv(p);
    tuple.key.mods |= modPrefix;
    KeyChord* slot = VECTOR_APPEND_SLOT(partialChords, KeyChord);
    *slot          = tuple;
    vectorAppend(tupleEnvs, &tupleEnv);
    return true;
}

static void
commitFinalPartial(KeyChord* currentPartial, Vector* partialChords, Vector* tupleEnvs)
{
    assert(currentPartial), assert(partialChords), assert(tupleEnvs);

    if (!stringIsEmpty(&currentPartial->key.repr))
    {
        KeyChord* slot = VECTOR_APPEND_SLOT(partialChords, KeyChord);
        *slot          = *currentPartial;
        appendEmptyEnv(tupleEnvs);
    }
    else
    {
        compilerFreeChord(currentPartial);
    }
}

static bool
parseExplicitArray(Parser* p)
{
    assert(p);

    KeyChord* uncommitted = parserCurrentChord(p);
    Modifier  modPrefix   = uncommitted->key.mods;
    Vector*   dest        = parserDest(p);

    compilerFreeChord(uncommitted);
    vectorRemove(dest, vectorLength(dest) - 1);
    parserAdvance(p);

    Vector partialChords = VECTOR_INIT(KeyChord);
    Vector tupleEnvs     = VECTOR_INIT(ArgEnvironment);
    bool   ok            = false;

    KeyChord currentPartial;
    compilerInitChord(&currentPartial);
    currentPartial.key.mods = modPrefix;

    Token* token = parserCurrentToken(p);
    while (!parserIsAtEnd(p) && token->type != TOKEN_RIGHT_BRACKET)
    {
        if (token->type == TOKEN_LEFT_PAREN)
        {
            if (!parseArrayTuple(p, &currentPartial, modPrefix, &partialChords, &tupleEnvs))
            {
                compilerFreeChord(&currentPartial);
                goto cleanup;
            }
        }
        else if (tokenIsModType(token->type))
        {
            accumulateModifier(&currentPartial.key.mods, token->type);
            parserAdvance(p);
        }
        else if (token->type == TOKEN_KEY || token->type == TOKEN_SPECIAL_KEY)
        {
            finalizePartialKey(
                p,
                token,
                &currentPartial,
                modPrefix,
                &partialChords,
                &tupleEnvs);
            parserAdvance(p);
        }
        else
        {
            parserErrorAtCurrent(p, "Unexpected token in chord array keys.");
            compilerFreeChord(&currentPartial);
            goto cleanup;
        }

        token = parserCurrentToken(p);
    }

    if (token->type == TOKEN_RIGHT_BRACKET)
    {
        commitFinalPartial(&currentPartial, &partialChords, &tupleEnvs);
        parserAdvance(p);
    }

    KeyChord sharedTemplate = parseSharedTemplate(p);
    if (parserHadError(p))
    {
        goto cleanup;
    }

    applySharedProperties(p, &partialChords, &tupleEnvs, &sharedTemplate);
    compilerFreeChord(&sharedTemplate);
    parserAllocChord(p);
    p->expect = parserNextChordExpectation(p);
    ok        = true;

cleanup:
    if (!ok)
    {
        vectorForEach(&partialChords, KeyChord, c)
        {
            compilerFreeChord(c);
        }
    }
    vectorForEach(&tupleEnvs, ArgEnvironment, env)
    {
        argEnvFree(env);
    }
    vectorFree(&partialChords);
    vectorFree(&tupleEnvs);
    return ok;
}

static void
keyCopyWithMod(Key* dest, const Key* src, Modifier modPrefix)
{
    assert(dest), assert(src);

    keyCopy(src, dest);
    dest->mods |= modPrefix;
}

static bool
parseImplicitArray(Parser* p)
{
    assert(p);

    KeyChord* uncommitted = parserCurrentChord(p);
    Modifier  modPrefix   = uncommitted->key.mods;
    Vector*   dest        = parserDest(p);

    compilerFreeChord(uncommitted);
    vectorRemove(dest, vectorLength(dest) - 1);
    parserAdvance(p);

    KeyChord sharedTemplate = parseSharedTemplate(p);
    if (parserHadError(p))
    {
        return false;
    }

    Vector* implicitKeys = parserImplicitKeys(p);
    vectorForEach(implicitKeys, const Key, implicitKey)
    {
        KeyChord* chord = VECTOR_APPEND_SLOT(dest, KeyChord);
        compilerInitChord(chord);

        keyCopyWithMod(&chord->key, implicitKey, modPrefix);
        applySharedPropertiesToChord(p, &sharedTemplate, chord, iter.index, NULL);
    }

    compilerFreeChord(&sharedTemplate);
    parserAllocChord(p);

    p->expect = parserNextChordExpectation(p);
    return true;
}

static void
synchronize(Parser* p)
{
    assert(p);

    p->panicMode = false;

    while (!parserIsAtEnd(p))
    {
        switch (p->current.type)
        {
        case TOKEN_LEFT_BRACKET: /* FALLTHROUGH */
        case TOKEN_LEFT_BRACE: parserAdvance(p); return;

        case TOKEN_COMM_INTERP: /* FALLTHROUGH */
        case TOKEN_COMMAND:
        case TOKEN_GOTO:
        {
            while (!parserIsAtEnd(p) && !parserCheck(p, TOKEN_COMMAND))
            {
                parserAdvance(p);
            }
            if (parserIsAtEnd(p)) return;

            parserAdvance(p);

            while (!parserIsAtEnd(p))
            {
                TokenType t = p->current.type;
                if (tokenIsModType(t) ||
                    t == TOKEN_KEY ||
                    t == TOKEN_SPECIAL_KEY ||
                    t == TOKEN_LEFT_BRACKET)
                {
                    return;
                }
                parserAdvance(p);
            }
            return;
        }

        default:
            parserAdvance(p);
            break;
        }
    }
}

static bool
parseSingleChord(Parser* p)
{
    while (!parserIsAtEnd(p))
    {
        Expectation got = tokenToExpectation(p->current.type);

        if (!(got & p->expect))
        {
            parserErrorUnexpected(p, p->expect, got);
            return false;
        }

        HandleResult result = handleCurrent(p);

        if (result.error) return false;

        if ((got == EXPECT_COMMAND || got == EXPECT_META) &&
            (result.next & EXPECT_KEY_START))
        {
            parserFinishChord(p);
            p->expect = result.next;
            return true;
        }

        p->expect = result.next;

        TokenType t = p->current.type;
        if (t == TOKEN_LEFT_BRACKET || t == TOKEN_ELLIPSIS ||
            tokenIsModType(t) || t == TOKEN_KEY || t == TOKEN_SPECIAL_KEY ||
            t == TOKEN_RIGHT_BRACE || t == TOKEN_EOF)
        {
            return true;
        }
    }

    return true;
}

static bool
parseImpl(Parser* p)
{
    assert(p);

    parserAdvance(p);
    p->expect = EXPECT_KEY_START | EXPECT_EOF;
    parserAllocChord(p);

    while (!parserIsAtEnd(p))
    {
        if (p->panicMode)
        {
            synchronize(p);
            if (parserIsAtEnd(p)) break;
        }

        TokenType t = p->current.type;

        bool ok;
        if (t == TOKEN_LEFT_BRACKET)
        {
            ok = parseExplicitArray(p);
        }
        else if (t == TOKEN_ELLIPSIS)
        {
            ok = parseImplicitArray(p);
        }
        else if (t == TOKEN_RIGHT_BRACE)
        {
            ok = parseRightBrace(p);
        }
        else if (tokenIsModType(t) || t == TOKEN_KEY || t == TOKEN_SPECIAL_KEY)
        {
            ok = parseSingleChord(p);
        }
        else
        {
            parserErrorUnexpected(p, p->expect, tokenToExpectation(t));
            ok = false;
        }

        if (!ok)
        {
            p->panicMode = true;
        }
    }

    if (!vectorIsEmpty(p->dest) && p->chord)
    {
        KeyChord* last = VECTOR_GET(p->dest, KeyChord, vectorLength(p->dest) - 1);
        if (last == p->chord && stringIsEmpty(&p->chord->key.repr))
        {
            compilerFreeChord(last);
            vectorRemove(p->dest, vectorLength(p->dest) - 1);
        }
    }

    return !p->hadError;
}

Vector
parse(Scanner* scanner, Menu* m)
{
    assert(scanner), assert(m);

    Parser p;
    parserInit(&p, scanner, m);

    if (!parseImplicitKeysImpl(&p, m->implicitArrayKeys))
    {
        parserFree(&p);
        return VECTOR_INIT(KeyChord);
    }

    if (!parseImpl(&p))
    {
        compilerFreeChordVector(&p.rootChords);
        parserFree(&p);
        return VECTOR_INIT(KeyChord);
    }

    parserFree(&p);
    return p.rootChords;
}
