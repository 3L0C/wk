#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* common includes */
#include "common/key_chord.h"
#include "common/menu.h"
#include "common/vector.h"

/* local includes */
#include "compiler.h"
#include "debug.h"
#include "expect.h"
#include "handler.h"
#include "parser.h"
#include "scanner.h"
#include "token.h"

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
    bool        hadError;
    bool        panicMode;
    bool        debug;
};

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
    p->chord = chord;
    return chord;
}

void
parserFinishChord(Parser* p)
{
    assert(p);

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

        Expectation got = tokenToExpectation(p->current.type);

        if (!(got & p->expect))
        {
            parserErrorUnexpected(p, p->expect, got);
            continue;
        }

        HandleResult result = handleCurrent(p);

        if (result.error)
        {
            p->panicMode = true;
            continue;
        }

        if ((got == EXPECT_COMMAND || got == EXPECT_META) &&
            (result.next & EXPECT_KEY_START))
        {
            parserFinishChord(p);
        }

        p->expect = result.next;
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
