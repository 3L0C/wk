#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* common includes */
#include "common/arena.h"
#include "common/key_chord.h"
#include "common/stack.h"
#include "common/string.h"
#include "common/vector.h"

/* local includes */
#include "args.h"
#include "expect.h"
#include "handler.h"
#include "parser.h"
#include "token.h"

static HandleResult handleArgs(Parser* p);
static HandleResult handleCommand(Parser* p);
static HandleResult handleDescription(Parser* p);
static HandleResult handleFlag(Parser* p);
static HandleResult handleFlagWithArg(Parser* p);
static HandleResult handleGoto(Parser* p);
static HandleResult handleHook(Parser* p);
static HandleResult handleKey(Parser* p);
static HandleResult handleLeftBrace(Parser* p);
static HandleResult handleLessThan(Parser* p);
static HandleResult handleMod(Parser* p);

static TokenHandler handlers[TOKEN_LAST] = {
    [TOKEN_MOD_CTRL]    = handleMod,
    [TOKEN_MOD_META]    = handleMod,
    [TOKEN_MOD_HYPER]   = handleMod,
    [TOKEN_MOD_SHIFT]   = handleMod,
    [TOKEN_LESS_THAN]   = handleLessThan,
    [TOKEN_KEY]         = handleKey,
    [TOKEN_SPECIAL_KEY] = handleKey,
    [TOKEN_DESCRIPTION] = handleDescription,
    [TOKEN_DESC_INTERP] = handleDescription,
    [TOKEN_BEFORE]      = handleHook,
    [TOKEN_AFTER]       = handleHook,
    [TOKEN_SYNC_BEFORE] = handleHook,
    [TOKEN_SYNC_AFTER]  = handleHook,
    [TOKEN_KEEP]        = handleFlag,
    [TOKEN_CLOSE]       = handleFlag,
    [TOKEN_INHERIT]     = handleFlag,
    [TOKEN_IGNORE]      = handleFlag,
    [TOKEN_UNHOOK]      = handleFlag,
    [TOKEN_DEFLAG]      = handleFlag,
    [TOKEN_NO_BEFORE]   = handleFlag,
    [TOKEN_NO_AFTER]    = handleFlag,
    [TOKEN_WRITE]       = handleFlag,
    [TOKEN_EXECUTE]     = handleFlag,
    [TOKEN_SYNC_CMD]    = handleFlag,
    [TOKEN_UNWRAP]      = handleFlag,
    [TOKEN_TITLE]       = handleFlagWithArg,
    [TOKEN_WRAP]        = handleFlagWithArg,
    [TOKEN_ARGS]        = handleArgs,
    [TOKEN_COMMAND]     = handleCommand,
    [TOKEN_COMM_INTERP] = handleCommand,
    [TOKEN_GOTO]        = handleGoto,
    [TOKEN_LEFT_BRACE]  = handleLeftBrace,
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

static void
spliceArgIntoVector(const Vector* arg, TokenType interpType, Vector* dest)
{
    assert(arg), assert(dest);

    vectorForEach(arg, const Token, argToken)
    {
        Token converted = *argToken;
        if (argToken->type == TOKEN_DESC_INTERP && interpType == TOKEN_COMM_INTERP)
        {
            converted.type = TOKEN_COMM_INTERP;
        }
        vectorAppend(dest, &converted);
    }
}

static void
tryResolveArgToken(Parser* p, const Token* token, TokenType interpType, Vector* dest)
{
    assert(p), assert(token), assert(dest);

    if (parserInTemplateContext(p))
    {
        vectorAppend(dest, token);
        return;
    }

    size_t  argIndex = strtoul(token->start, NULL, 10);
    Vector* arg      = argEnvStackLookup(parserArgEnvStack(p), argIndex);

    if (arg && !vectorIsEmpty(arg))
    {
        spliceArgIntoVector(arg, interpType, dest);
        return;
    }

    parserDebugAt(p, (Token*)token, "Argument $%zu not defined.", argIndex);
}

static bool
collectDescriptionTokens(Parser* p, Vector* tokens, TokenType interpType)
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
        case TOKEN_THIS_DESC:
        case TOKEN_THIS_DESC_UPPER_FIRST:
        case TOKEN_THIS_DESC_LOWER_FIRST:
        case TOKEN_THIS_DESC_UPPER_ALL:
        case TOKEN_THIS_DESC_LOWER_ALL:
            vectorAppend(tokens, token);
            parserAdvance(p);
            break;

        case TOKEN_ARG_POSITION:
            tryResolveArgToken(p, token, interpType, tokens);
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

static HandleResult
handleArgs(Parser* p)
{
    assert(p);

    parserAdvance(p);

    ArgEnvironment env;
    argEnvInit(&env);

    while (!parserIsAtEnd(p))
    {
        Token* token = parserCurrentToken(p);
        if (token->type != TOKEN_DESCRIPTION && token->type != TOKEN_DESC_INTERP)
            break;

        Vector arg = VECTOR_INIT(Token);
        if (!collectDescriptionTokens(p, &arg, TOKEN_DESC_INTERP))
        {
            vectorFree(&arg);
            argEnvFree(&env);
            return handleResultError();
        }

        argEnvAddArg(&env, &arg);
    }

    if (argEnvIsEmpty(&env))
    {
        parserErrorAtCurrent(p, "+args requires at least one argument.");
        argEnvFree(&env);
        return handleResultError();
    }

    stackPush(parserArgEnvStack(p), &env);
    parserSetChordPushedEnv(p, true);

    return handleResultOk(EXPECT_AFTER_FLAG);
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

        case TOKEN_ARG_POSITION:
            tryResolveArgToken(p, token, TOKEN_COMM_INTERP, tokens);
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
handleDescription(Parser* p)
{
    assert(p);

    KeyChord* chord  = parserCurrentChord(p);
    Vector*   tokens = propVector(chord, KC_PROP_DESCRIPTION);

    if (!collectDescriptionTokens(p, tokens, TOKEN_DESC_INTERP))
    {
        return handleResultError();
    }

    return handleResultOk(EXPECT_AFTER_DESC);
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

    PropId    propId;
    TokenType tokenType = token->type;
    switch (tokenType)
    {
    case TOKEN_TITLE: propId = KC_PROP_TITLE; break;
    case TOKEN_WRAP: propId = KC_PROP_WRAP_CMD; break;
    default:
        parserErrorAtCurrent(p, "Unexpected flag with argument.");
        return handleResultError();
    }

    parserAdvance(p);

    Token*  next   = parserCurrentToken(p);
    Vector* tokens = propVector(chord, propId);

    if (next->type == TOKEN_DESCRIPTION || next->type == TOKEN_DESC_INTERP)
    {
        TokenType interpType = (propId == KC_PROP_WRAP_CMD)
                                   ? TOKEN_COMM_INTERP
                                   : TOKEN_DESC_INTERP;
        vectorClear(tokens);
        if (!collectDescriptionTokens(p, tokens, interpType))
        {
            return handleResultError();
        }
    }
    else if (tokenType == TOKEN_TITLE)
    {
        static const Token sentinel = { .type = TOKEN_EMPTY };
        vectorClear(tokens);
        vectorAppend(tokens, &sentinel);
    }

    return handleResultOk(EXPECT_AFTER_FLAG);
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
    if (!collectDescriptionTokens(p, tokens, TOKEN_DESC_INTERP))
    {
        return handleResultError();
    }

    return handleResultOk(parserNextChordExpectation(p));
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
handleLeftBrace(Parser* p)
{
    assert(p);

    KeyChord* chord = parserCurrentChord(p);

    if (propHasContent(chord, KC_PROP_GOTO))
    {
        parserErrorAtCurrent(p, "Cannot mix prefix and @goto.");
        return handleResultError();
    }

    size_t currentDepth = parserDepth(p);
    parserSetPushedEnvAtDepth(p, currentDepth, parserChordPushedEnv(p));
    parserSetChordPushedEnv(p, false);

    parserPushState(p);
    parserSetDest(p, parserChildVector(p, parserDepth(p)));
    parserAdvance(p);
    parserAllocChord(p);

    return handleResultOk(EXPECT_KEY_START | EXPECT_RBRACE);
}

static Modifier
tokenToModifier(TokenType type)
{
    switch (type)
    {
    case TOKEN_MOD_CTRL: return MOD_CTRL;
    case TOKEN_MOD_META: return MOD_META;
    case TOKEN_MOD_HYPER: return MOD_HYPER;
    case TOKEN_MOD_SHIFT: return MOD_SHIFT;
    default: return MOD_NONE;
    }
}

static void
appendKeyOption(Vector* options, Parser* p, Token* token, Modifier mods)
{
    assert(options), assert(p), assert(token);

    Key key;
    keyInit(&key);
    key.mods = mods;

    Arena* arena = parserArena(p);
    if (token->type == TOKEN_SPECIAL_KEY)
    {
        key.special = token->special;
        key.repr    = stringFromCString(arena, specialKeyRepr(key.special));
    }
    else
    {
        key.repr = stringMake(arena, token->start, token->length);
    }

    vectorAppend(options, &key);
}

static void
appendImplicitKeysAsOptions(Vector* options, Parser* p, Modifier modPrefix)
{
    assert(options), assert(p);

    Vector* implicitKeys = parserImplicitKeys(p);
    Arena*  arena        = parserArena(p);

    vectorForEach(implicitKeys, const Key, implicitKey)
    {
        Key copy;
        keyInit(&copy);
        copy.mods    = implicitKey->mods | modPrefix;
        copy.special = implicitKey->special;
        if (implicitKey->special != SPECIAL_KEY_NONE)
        {
            copy.repr = stringFromCString(arena, specialKeyRepr(copy.special));
        }
        else
        {
            copy.repr = stringMake(arena, implicitKey->repr.data, implicitKey->repr.length);
        }
        vectorAppend(options, &copy);
    }
}

static bool
keyIsBoundInDest(Vector* dest, const Key* candidate)
{
    assert(dest), assert(candidate);

    vectorForEach(dest, const KeyChord, chord)
    {
        if (keyIsEqual(&chord->key, candidate)) return true;
    }
    return false;
}

static HandleResult
handleLessThan(Parser* p)
{
    assert(p);

    KeyChord* chord     = parserCurrentChord(p);
    Modifier  modPrefix = chord->key.mods;
    chord->key.mods     = MOD_NONE;

    Vector   options   = VECTOR_INIT(Key);
    Modifier localMods = MOD_NONE;

    parserAdvance(p);

    while (!parserIsAtEnd(p))
    {
        Token* token = parserCurrentToken(p);

        switch (token->type)
        {
        case TOKEN_MOD_CTRL: /* FALLTHROUGH */
        case TOKEN_MOD_META:
        case TOKEN_MOD_HYPER:
        case TOKEN_MOD_SHIFT:
            localMods |= tokenToModifier(token->type);
            parserAdvance(p);
            break;

        case TOKEN_KEY: /* FALLTHROUGH */
        case TOKEN_SPECIAL_KEY:
            appendKeyOption(&options, p, token, modPrefix | localMods);
            localMods = MOD_NONE;
            parserAdvance(p);
            break;

        case TOKEN_ELLIPSIS:
            appendImplicitKeysAsOptions(&options, p, modPrefix | localMods);
            localMods = MOD_NONE;
            parserAdvance(p);
            break;

        case TOKEN_GREATER_THAN:
            goto resolve;

        default:
            vectorFree(&options);
            parserErrorAtCurrent(p, "Unexpected token in key options.");
            return handleResultError();
        }
    }

    vectorFree(&options);
    parserErrorAtCurrent(p, "Unterminated key options, expected '>'.");
    return handleResultError();

resolve:
    if (vectorIsEmpty(&options))
    {
        vectorFree(&options);
        parserErrorAtCurrent(p, "Empty key options '<>'.");
        return handleResultError();
    }

    Key*    winner = NULL;
    Vector* dest   = parserDest(p);

    vectorForEach(&options, Key, candidate)
    {
        if (!keyIsBoundInDest(dest, candidate))
        {
            winner = candidate;
            break;
        }
    }

    if (!winner)
    {
        vectorFree(&options);
        parserErrorAtCurrent(p, "All key options are already bound.");
        return handleResultError();
    }

    keyCopy(winner, &chord->key);

    vectorFree(&options);
    parserAdvance(p);
    return handleResultOk(EXPECT_DESC);
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
    return handleResultOk(EXPECT_MOD | EXPECT_KEY | EXPECT_ELLIPSIS | EXPECT_LESS_THAN);
}
