#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* common includes */
#include "common/arena.h"
#include "common/key_chord.h"
#include "common/property.h"
#include "common/span.h"
#include "common/stack.h"
#include "common/string.h"
#include "common/vector.h"

/* local includes */
#include "compiler.h"
#include "expect.h"
#include "handler.h"
#include "parser.h"
#include "token.h"

typedef struct
{
    const char* start;
    size_t      length;
} InternedIndex;

static InternedIndex indexInterns[256] = { 0 };

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

static HandleResult handleMod(Parser* p);
static HandleResult handleKey(Parser* p);
static HandleResult handleDescription(Parser* p);
static HandleResult handleHook(Parser* p);
static HandleResult handleFlag(Parser* p);
static HandleResult handleFlagWithArg(Parser* p);
static HandleResult handleCommand(Parser* p);
static HandleResult handleGoto(Parser* p);
static HandleResult handleLeftBrace(Parser* p);
static HandleResult handleRightBrace(Parser* p);
static HandleResult handleLeftBracket(Parser* p);
static HandleResult handleEllipsis(Parser* p);

static bool parseChordContent(Parser* p, KeyChord* chord, Expectation terminator);

static TokenHandler handlers[TOKEN_LAST] = {
    [TOKEN_MOD_CTRL]     = handleMod,
    [TOKEN_MOD_META]     = handleMod,
    [TOKEN_MOD_HYPER]    = handleMod,
    [TOKEN_MOD_SHIFT]    = handleMod,
    [TOKEN_KEY]          = handleKey,
    [TOKEN_SPECIAL_KEY]  = handleKey,
    [TOKEN_DESCRIPTION]  = handleDescription,
    [TOKEN_DESC_INTERP]  = handleDescription,
    [TOKEN_BEFORE]       = handleHook,
    [TOKEN_AFTER]        = handleHook,
    [TOKEN_SYNC_BEFORE]  = handleHook,
    [TOKEN_SYNC_AFTER]   = handleHook,
    [TOKEN_KEEP]         = handleFlag,
    [TOKEN_CLOSE]        = handleFlag,
    [TOKEN_INHERIT]      = handleFlag,
    [TOKEN_IGNORE]       = handleFlag,
    [TOKEN_UNHOOK]       = handleFlag,
    [TOKEN_DEFLAG]       = handleFlag,
    [TOKEN_NO_BEFORE]    = handleFlag,
    [TOKEN_NO_AFTER]     = handleFlag,
    [TOKEN_WRITE]        = handleFlag,
    [TOKEN_EXECUTE]      = handleFlag,
    [TOKEN_SYNC_CMD]     = handleFlag,
    [TOKEN_UNWRAP]       = handleFlag,
    [TOKEN_TITLE]        = handleFlagWithArg,
    [TOKEN_WRAP]         = handleFlagWithArg,
    [TOKEN_COMMAND]      = handleCommand,
    [TOKEN_COMM_INTERP]  = handleCommand,
    [TOKEN_GOTO]         = handleGoto,
    [TOKEN_LEFT_BRACE]   = handleLeftBrace,
    [TOKEN_RIGHT_BRACE]  = handleRightBrace,
    [TOKEN_LEFT_BRACKET] = handleLeftBracket,
    [TOKEN_ELLIPSIS]     = handleEllipsis,
};

TokenHandler
getHandler(TokenType type)
{
    if (type >= TOKEN_LAST) return NULL;
    return handlers[type];
}

HandleResult
handleCurrent(Parser* p)
{
    assert(p);

    Token*       token   = parserCurrentToken(p);
    TokenHandler handler = getHandler(token->type);
    if (!handler)
    {
        parserErrorAtCurrent(p, "Unexpected token type: %s.", tokenLiteral(token->type));
        return handleResultError();
    }

    return handler(p);
}

static bool
collectDescriptionTokens(Parser* p, Vector* tokens)
{
    assert(p), assert(tokens);

    while (!parserIsAtEnd(p))
    {
        Token* token = parserCurrentToken(p);

        switch (token->type)
        {
        case TOKEN_DESC_INTERP: /* FALLTHROUGH */
        case TOKEN_THIS_KEY:
        case TOKEN_INDEX:
        case TOKEN_INDEX_ONE:
        case TOKEN_USER_VAR:
        case TOKEN_WRAP_CMD_INTERP:
            vectorAppend(tokens, token);
            parserAdvance(p);
            break;

        case TOKEN_DESCRIPTION:
            vectorAppend(tokens, token);
            parserAdvance(p);
            return true;

        default:
            parserErrorAtCurrent(p, "Unexpected token in description.");
            return false;
        }
    }

    parserErrorAtCurrent(p, "Unterminated description.");
    return false;
}

static bool
collectCommandTokens(Parser* p, Vector* tokens)
{
    assert(p), assert(tokens);

    while (!parserIsAtEnd(p))
    {
        Token* token = parserCurrentToken(p);

        switch (token->type)
        {
        case TOKEN_COMM_INTERP: /* FALLTHROUGH */
        case TOKEN_THIS_KEY:
        case TOKEN_INDEX:
        case TOKEN_INDEX_ONE:
        case TOKEN_USER_VAR:
        case TOKEN_WRAP_CMD_INTERP:
        case TOKEN_THIS_DESC:
        case TOKEN_THIS_DESC_UPPER_FIRST:
        case TOKEN_THIS_DESC_LOWER_FIRST:
        case TOKEN_THIS_DESC_UPPER_ALL:
        case TOKEN_THIS_DESC_LOWER_ALL:
            vectorAppend(tokens, token);
            parserAdvance(p);
            break;

        case TOKEN_COMMAND:
            vectorAppend(tokens, token);
            parserAdvance(p);
            return true;

        default:
            parserErrorAtCurrent(p, "Unexpected token in command.");
            return false;
        }
    }

    parserErrorAtCurrent(p, "Unterminated command.");
    return false;
}

static HandleResult
handleMod(Parser* p)
{
    assert(p);

    KeyChord* chord = parserCurrentChord(p);
    Token*    token = parserCurrentToken(p);

    switch (token->type)
    {
    case TOKEN_MOD_CTRL: chord->key.mods |= MOD_CTRL; break;
    case TOKEN_MOD_META: chord->key.mods |= MOD_META; break;
    case TOKEN_MOD_HYPER: chord->key.mods |= MOD_HYPER; break;
    case TOKEN_MOD_SHIFT: chord->key.mods |= MOD_SHIFT; break;
    default: break;
    }

    parserAdvance(p);
    return handleResultOk(EXPECT_MOD | EXPECT_KEY | EXPECT_ELLIPSIS);
}

static HandleResult
handleKey(Parser* p)
{
    assert(p);

    KeyChord* chord = parserCurrentChord(p);
    Token*    token = parserCurrentToken(p);
    Arena*    arena = parserArena(p);

    if (token->type == TOKEN_SPECIAL_KEY)
    {
        chord->key.special = token->special;
        chord->key.repr    = stringFromCString(arena, specialKeyRepr(chord->key.special));
    }
    else
    {
        chord->key.repr = stringMake(arena, token->start, token->length);
    }

    parserAdvance(p);
    return handleResultOk(EXPECT_DESC);
}

static HandleResult
handleDescription(Parser* p)
{
    assert(p);

    KeyChord* chord  = parserCurrentChord(p);
    Vector*   tokens = propVector(chord, KC_PROP_DESCRIPTION);

    if (!collectDescriptionTokens(p, tokens))
    {
        return handleResultError();
    }

    return handleResultOk(EXPECT_AFTER_DESC);
}

static HandleResult
handleHook(Parser* p)
{
    assert(p);

    KeyChord* chord = parserCurrentChord(p);
    Token*    token = parserCurrentToken(p);

    if (propHasContent(chord, KC_PROP_GOTO))
    {
        parserErrorAtCurrent(p, "Cannot mix hooks and @goto.");
        return handleResultError();
    }

    PropId    propId;
    ChordFlag flag = FLAG_NONE;

    switch (token->type)
    {
    case TOKEN_BEFORE:
        propId = KC_PROP_BEFORE;
        break;
    case TOKEN_AFTER:
        propId = KC_PROP_AFTER;
        break;
    case TOKEN_SYNC_BEFORE:
        propId = KC_PROP_BEFORE;
        flag   = FLAG_SYNC_BEFORE;
        break;
    case TOKEN_SYNC_AFTER:
        propId = KC_PROP_AFTER;
        flag   = FLAG_SYNC_AFTER;
        break;
    default:
        parserErrorAtCurrent(p, "Unexpected hook type.");
        return handleResultError();
    }

    parserAdvance(p);

    if (flag != FLAG_NONE) chord->flags |= flag;

    Vector* tokens = propVector(chord, propId);
    if (!collectCommandTokens(p, tokens))
    {
        return handleResultError();
    }

    return handleResultOk(EXPECT_AFTER_HOOK);
}

static HandleResult
handleFlag(Parser* p)
{
    assert(p);

    KeyChord* chord = parserCurrentChord(p);
    Token*    token = parserCurrentToken(p);

    switch (token->type)
    {
    case TOKEN_KEEP: chord->flags |= FLAG_KEEP; break;
    case TOKEN_CLOSE: chord->flags |= FLAG_CLOSE; break;
    case TOKEN_INHERIT: chord->flags |= FLAG_INHERIT; break;
    case TOKEN_IGNORE: chord->flags |= FLAG_IGNORE; break;
    case TOKEN_UNHOOK: chord->flags |= FLAG_UNHOOK; break;
    case TOKEN_DEFLAG: chord->flags |= FLAG_DEFLAG; break;
    case TOKEN_NO_BEFORE: chord->flags |= FLAG_NO_BEFORE; break;
    case TOKEN_NO_AFTER: chord->flags |= FLAG_NO_AFTER; break;
    case TOKEN_WRITE: chord->flags |= FLAG_WRITE; break;
    case TOKEN_EXECUTE: chord->flags |= FLAG_EXECUTE; break;
    case TOKEN_SYNC_CMD: chord->flags |= FLAG_SYNC_COMMAND; break;
    case TOKEN_UNWRAP: chord->flags |= FLAG_UNWRAP; break;
    default:
        parserErrorAtCurrent(p, "Unexpected flag type.");
        return handleResultError();
    }

    parserAdvance(p);
    return handleResultOk(EXPECT_AFTER_FLAG);
}

static HandleResult
handleFlagWithArg(Parser* p)
{
    assert(p);

    KeyChord* chord = parserCurrentChord(p);
    Token*    token = parserCurrentToken(p);

    PropId propId;
    switch (token->type)
    {
    case TOKEN_TITLE: propId = KC_PROP_TITLE; break;
    case TOKEN_WRAP: propId = KC_PROP_WRAP_CMD; break;
    default:
        parserErrorAtCurrent(p, "Unexpected flag with argument.");
        return handleResultError();
    }

    parserAdvance(p);

    Token* next = parserCurrentToken(p);
    if (next->type == TOKEN_DESCRIPTION || next->type == TOKEN_DESC_INTERP)
    {
        Vector* tokens = propVector(chord, propId);
        vectorClear(tokens);
        if (!collectDescriptionTokens(p, tokens))
        {
            return handleResultError();
        }
    }

    return handleResultOk(EXPECT_AFTER_FLAG);
}

static HandleResult
handleCommand(Parser* p)
{
    assert(p);

    KeyChord* chord = parserCurrentChord(p);

    if (propHasContent(chord, KC_PROP_GOTO))
    {
        parserErrorAtCurrent(p, "Cannot mix commands and @goto.");
        return handleResultError();
    }

    Vector* tokens = propVector(chord, KC_PROP_COMMAND);
    if (!collectCommandTokens(p, tokens))
    {
        return handleResultError();
    }

    return handleResultOk(parserNextChordExpectation(p));
}

static HandleResult
handleGoto(Parser* p)
{
    assert(p);

    KeyChord* chord = parserCurrentChord(p);

    if (propHasContent(chord, KC_PROP_BEFORE) || propHasContent(chord, KC_PROP_AFTER))
    {
        parserErrorAtCurrent(p, "Cannot mix @goto and hooks.");
        return handleResultError();
    }

    if (propHasContent(chord, KC_PROP_COMMAND))
    {
        parserErrorAtCurrent(p, "Cannot mix @goto and commands.");
        return handleResultError();
    }

    parserAdvance(p);

    Vector* tokens = propVector(chord, KC_PROP_GOTO);
    if (!collectDescriptionTokens(p, tokens))
    {
        return handleResultError();
    }

    return handleResultOk(parserNextChordExpectation(p));
}

static int
compareSizeT(const void* a, const void* b)
{
    size_t x = *(const size_t*)a;
    size_t y = *(const size_t*)b;
    return (x > y) - (x < y);
}

static void
deduplicateTempVector(Vector* chords)
{
    assert(chords);

    if (vectorIsEmpty(chords)) return;

    Stack stack = STACK_INIT(size_t);

    vectorForRange(chords, KeyChord, outerChord, 0, vectorLength(chords) - 1)
    {
        VectorIterator* outerIter = &iter;
        size_t          swapIdx   = outerIter->index;
        vectorForEachFrom(chords, KeyChord, innerChord, outerIter->index + 1)
        {
            if (keyIsEqual(&outerChord->key, &innerChord->key))
            {
                stackPush(&stack, &iter.index);
                swapIdx = iter.index;
            }
        }
        vectorSwap(chords, outerIter->index, swapIdx);
    }

    vectorSort(&stack, compareSizeT);

    while (!stackIsEmpty(&stack))
    {
        size_t    index = *(STACK_PEEK(&stack, size_t));
        KeyChord* dup   = VECTOR_GET(chords, KeyChord, index);
        compilerFreeChord(dup);
        vectorRemove(chords, index);

        while (!stackIsEmpty(&stack) && index == *(STACK_PEEK(&stack, size_t)))
        {
            stackPop(&stack);
        }
    }

    stackFree(&stack);
}

static HandleResult
handleLeftBrace(Parser* p)
{
    assert(p);

    KeyChord* chord = parserCurrentChord(p);

    if (propHasContent(chord, KC_PROP_GOTO))
    {
        parserErrorAtCurrent(p, "Cannot mix prefix and @goto.");
        return handleResultError();
    }

    parserPushState(p);
    parserSetDest(p, parserChildVector(p, parserDepth(p)));
    parserAdvance(p);
    parserAllocChord(p);

    return handleResultOk(EXPECT_KEY_START | EXPECT_RBRACE);
}

static HandleResult
handleRightBrace(Parser* p)
{
    assert(p);

    if (parserDepth(p) == 0)
    {
        parserErrorAtCurrent(p, "Unexpected '}'.");
        return handleResultError();
    }

    Vector* dest = parserDest(p);

    if (vectorIsEmpty(dest))
    {
        parserErrorAtCurrent(p, "Empty prefix.");
        return handleResultError();
    }

    KeyChord* lastChord = parserCurrentChord(p);
    if (lastChord && stringIsEmpty(&lastChord->key.repr))
    {
        compilerFreeChord(lastChord);
        vectorRemove(dest, vectorLength(dest) - 1);
    }

    deduplicateTempVector(dest);

    KeyChord* parentChord = parserSavedChord(p, parserDepth(p) - 1);

    parentChord->keyChords = SPAN_FROM_VECTOR(parserArena(p), dest, KeyChord);

    if (!parserPopState(p))
    {
        parserErrorAtCurrent(p, "State stack underflow.");
        return handleResultError();
    }

    size_t depth = parserDepth(p);
    parserSetDest(p, parserSavedDest(p, depth));
    parserAdvance(p);
    parserAllocChord(p);

    return handleResultOk(parserNextChordExpectation(p));
}

static bool
parseKeyIntoChord(Parser* p, KeyChord* chord)
{
    assert(p), assert(chord);

    Token* token = parserCurrentToken(p);

    /* Parse modifiers */
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

    if (!parseChordContent(p, tuple, EXPECT_RPAREN))
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

static TokenType
getInterpTypeForProperty(PropId id)
{
    switch (id)
    {
    case KC_PROP_COMMAND:
    case KC_PROP_BEFORE:
    case KC_PROP_AFTER:
    case KC_PROP_WRAP_CMD:
        return TOKEN_COMM_INTERP;
    default:
        return TOKEN_DESC_INTERP;
    }
}

static void
applySharedProperties(Arena* arena, const KeyChord* from, KeyChord* to, size_t index)
{
    assert(arena), assert(from), assert(to);

    if (to->flags == FLAG_NONE) to->flags = from->flags;

    for (PropId id = 0; id < KC_PROP_COUNT; id++)
    {
        const Property* fromProp = propGetConst(from, id);
        Property*       toProp   = propGet(to, id);

        if (propertyHasContent(fromProp) && !propertyHasContent(toProp))
        {
            if (fromProp->type == PROP_TYPE_ARRAY)
            {
                PROP_SET_TYPE(toProp, ARRAY);
                toProp->value.as_array = VECTOR_INIT(Token);

                const Vector* fromVec    = &fromProp->value.as_array;
                Vector*       toVec      = &toProp->value.as_array;
                TokenType     interpType = getInterpTypeForProperty(id);

                vectorForEach(fromVec, const Token, token)
                {
                    if (token->type == TOKEN_INDEX || token->type == TOKEN_INDEX_ONE)
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
        }
    }
}

static bool
parseChordContent(Parser* p, KeyChord* chord, Expectation terminator)
{
    assert(p), assert(chord);

    KeyChord* saved = parserCurrentChord(p);
    parserSetChord(p, chord);

    Expectation expect = EXPECT_DESC;

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
parseSharedTemplate(Parser* p, KeyChord* tmpl)
{
    assert(p), assert(tmpl);

    if (!parseChordContent(p, tmpl, EXPECT_KEY_START | EXPECT_RBRACE | EXPECT_EOF))
    {
        compilerFreeChord(tmpl);
        return false;
    }

    return true;
}

static HandleResult
handleLeftBracket(Parser* p)
{
    assert(p);

    KeyChord* uncommitted = parserCurrentChord(p);
    Modifier  modPrefix   = uncommitted->key.mods;
    Vector*   dest        = parserDest(p);

    compilerFreeChord(uncommitted);
    vectorRemove(dest, vectorLength(dest) - 1);
    parserAdvance(p);

    Vector partialChords = VECTOR_INIT(KeyChord);

    KeyChord currentPartial;
    compilerInitChord(&currentPartial);
    currentPartial.key.mods = modPrefix;

    Token* token = parserCurrentToken(p);
    while (!parserIsAtEnd(p))
    {
        if (token->type == TOKEN_RIGHT_BRACKET)
        {
            if (!stringIsEmpty(&currentPartial.key.repr))
            {
                KeyChord* slot = VECTOR_APPEND_SLOT(&partialChords, KeyChord);
                *slot          = currentPartial;
            }
            else
            {
                compilerFreeChord(&currentPartial);
            }
            parserAdvance(p);
            break;
        }

        if (token->type == TOKEN_LEFT_PAREN)
        {
            if (!stringIsEmpty(&currentPartial.key.repr))
            {
                KeyChord* slot = VECTOR_APPEND_SLOT(&partialChords, KeyChord);
                *slot          = currentPartial;
                compilerInitChord(&currentPartial);
                currentPartial.key.mods = modPrefix;
            }

            KeyChord tuple;
            if (!parseTuple(p, &tuple))
            {
                compilerFreeChord(&currentPartial);
                vectorForEach(&partialChords, KeyChord, c) { compilerFreeChord(c); }
                vectorFree(&partialChords);
                return handleResultError();
            }
            tuple.key.mods |= modPrefix;
            KeyChord* slot = VECTOR_APPEND_SLOT(&partialChords, KeyChord);
            *slot          = tuple;
        }
        else if (tokenIsModType(token->type))
        {
            switch (token->type)
            {
            case TOKEN_MOD_CTRL: currentPartial.key.mods |= MOD_CTRL; break;
            case TOKEN_MOD_META: currentPartial.key.mods |= MOD_META; break;
            case TOKEN_MOD_HYPER: currentPartial.key.mods |= MOD_HYPER; break;
            case TOKEN_MOD_SHIFT: currentPartial.key.mods |= MOD_SHIFT; break;
            default: break;
            }
            parserAdvance(p);
        }
        else if (token->type == TOKEN_KEY || token->type == TOKEN_SPECIAL_KEY)
        {
            Arena* arena = parserArena(p);
            if (token->type == TOKEN_SPECIAL_KEY)
            {
                currentPartial.key.special = token->special;
                currentPartial.key.repr    = stringFromCString(
                    arena,
                    specialKeyRepr(currentPartial.key.special));
            }
            else
            {
                currentPartial.key.repr = stringMake(arena, token->start, token->length);
            }
            parserAdvance(p);

            KeyChord* slot = VECTOR_APPEND_SLOT(&partialChords, KeyChord);
            *slot          = currentPartial;
            compilerInitChord(&currentPartial);
            currentPartial.key.mods = modPrefix;
        }
        else
        {
            parserErrorAtCurrent(p, "Unexpected token in chord array keys.");
            compilerFreeChord(&currentPartial);
            vectorForEach(&partialChords, KeyChord, c) { compilerFreeChord(c); }
            vectorFree(&partialChords);
            return handleResultError();
        }

        token = parserCurrentToken(p);
    }

    KeyChord sharedTemplate;
    compilerInitChord(&sharedTemplate);

    if (!parseSharedTemplate(p, &sharedTemplate))
    {
        vectorForEach(&partialChords, KeyChord, c) { compilerFreeChord(c); }
        vectorFree(&partialChords);
        return handleResultError();
    }

    Arena* arena = parserArena(p);
    vectorForEach(&partialChords, KeyChord, partial)
    {
        applySharedProperties(arena, &sharedTemplate, partial, iter.index);
        KeyChord* slot = VECTOR_APPEND_SLOT(dest, KeyChord);
        *slot          = *partial;
    }

    compilerFreeChord(&sharedTemplate);
    vectorFree(&partialChords);
    parserAllocChord(p);

    return handleResultOk(parserNextChordExpectation(p));
}

static HandleResult
handleEllipsis(Parser* p)
{
    assert(p);

    KeyChord* uncommitted = parserCurrentChord(p);
    Modifier  modPrefix   = uncommitted->key.mods;
    Vector*   dest        = parserDest(p);

    compilerFreeChord(uncommitted);
    vectorRemove(dest, vectorLength(dest) - 1);
    parserAdvance(p);

    KeyChord sharedTemplate;
    compilerInitChord(&sharedTemplate);

    if (!parseSharedTemplate(p, &sharedTemplate))
    {
        return handleResultError();
    }

    Vector* implicitKeys = parserImplicitKeys(p);
    Arena*  arena        = parserArena(p);
    vectorForEach(implicitKeys, const Key, implicitKey)
    {
        KeyChord* chord = VECTOR_APPEND_SLOT(dest, KeyChord);
        compilerInitChord(chord);

        chord->key.mods    = modPrefix | implicitKey->mods;
        chord->key.special = implicitKey->special;
        chord->key.repr    = implicitKey->repr;

        applySharedProperties(arena, &sharedTemplate, chord, iter.index);
    }

    compilerFreeChord(&sharedTemplate);
    parserAllocChord(p);

    return handleResultOk(parserNextChordExpectation(p));
}
