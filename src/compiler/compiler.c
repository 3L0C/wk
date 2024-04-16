#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* common includes */
#include "common/common.h"
#include "common/key_chord.h"
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
static KeyChord nullKeyChord = {0};

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
addMod(TokenType type, KeyChordMods* mod)
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
compileMods(Compiler* compiler, KeyChord* keyChord)
{
    assert(compiler && keyChord);
    if (compiler->panicMode) return;

    TokenType type = currentType(compiler);
    while (isTokenModType(type))
    {
        addMod(type, &keyChord->mods);
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
compileKeyFromToken(Compiler* compiler, KeyChord* keyChord)
{
    assert(compiler && keyChord);
    if (compiler->panicMode) return;

    Token* token = currentToken(compiler);
    String result = {0};
    initString(&result);
    appendToString(&result, token->start, token->length);
    keyChord->key = result.string;
    disownString(&result);
}

static void
compileKey(Compiler* compiler, KeyChord* keyChord)
{
    assert(compiler && keyChord);
    if (compiler->panicMode) return;

    if (!isKey(compiler)) return errorAtCurrent(compiler, "Expected key or special.");
    compileKeyFromToken(compiler, keyChord);
    consume(compiler, currentType(compiler), "Expected key or special.");
}

static void
compileDescription(Compiler* compiler, KeyChord* keyChord)
{
    assert(compiler && keyChord);
    if (compiler->panicMode) return;

    if (!checkCompiler(compiler, TOKEN_DESC_INTERP) &&
        !checkCompiler(compiler, TOKEN_DESCRIPTION))
    {
        errorAtCurrent(compiler, "Expected description.");
        return;
    }

    String result = {0};
    initString(&result);
    while (!checkCompiler(compiler, TOKEN_EOF))
    {
        Token* token = currentToken(compiler);
        switch (token->type)
        {
        case TOKEN_THIS_KEY: appendToString(&result, keyChord->key, strlen(keyChord->key)); break;
        case TOKEN_INDEX: appendUInt32ToString(&result, compiler->index); break;
        case TOKEN_INDEX_ONE: appendUInt32ToString(&result, compiler->index + 1); break;
        case TOKEN_DESC_INTERP: /* FALLTHROUGH */
        case TOKEN_DESCRIPTION: appendToString(&result, token->start, token->length); break;
        default: errorAtCurrent(compiler, "Malfromed description."); goto fail;
        }
        if (checkCompiler(compiler, TOKEN_DESCRIPTION)) break;
        advanceCompiler(compiler);
    }

    keyChord->description = result.string;
    disownString(&result);
    consume(compiler, TOKEN_DESCRIPTION, "Expect description.");
    return;

fail:
    freeString(&result);
    return;
}

static void
compileCommand(Compiler* compiler, KeyChord* keyChord, char** dest)
{
    assert(compiler && keyChord && dest);
    if (compiler->panicMode) return;

    /* No longer in a chord expression but not an error */
    if (checkCompiler(compiler, TOKEN_RIGHT_PAREN)) return;

    if (!checkCompiler(compiler, TOKEN_COMM_INTERP) &&
        !checkCompiler(compiler, TOKEN_COMMAND))
    {
        errorAtCurrent(compiler, "Expected command.");
        return;
    }

    String result = {0};
    initString(&result);
    while (!compilerIsAtEnd(compiler))
    {
        Token* token = currentToken(compiler);
        switch (token->type)
        {
        case TOKEN_THIS_KEY:
        {
            appendToString(&result, keyChord->key, strlen(keyChord->key));
            break;
        }
        case TOKEN_THIS_DESC:
        {
            appendToString(&result, keyChord->description, strlen(keyChord->description));
            break;
        }
        case TOKEN_THIS_DESC_UPPER_FIRST:
        {
            appendToStringWithState(
                &result, keyChord->description,
                strlen(keyChord->description), STRING_APPEND_UPPER_FIRST
            );
            break;
        }
        case TOKEN_THIS_DESC_LOWER_FIRST:
        {
            appendToStringWithState(
                &result, keyChord->description,
                strlen(keyChord->description), STRING_APPEND_LOWER_FIRST
            );
            break;
        }
        case TOKEN_THIS_DESC_UPPER_ALL:
        {
            appendToStringWithState(
                &result, keyChord->description,
                strlen(keyChord->description), STRING_APPEND_UPPER_ALL
            );
            break;
        }
        case TOKEN_THIS_DESC_LOWER_ALL:
        {
            appendToStringWithState(
                &result, keyChord->description,
                strlen(keyChord->description), STRING_APPEND_LOWER_ALL
            );
            break;
        }
        case TOKEN_INDEX:
        {
            appendUInt32ToString(&result, compiler->index);
            break;
        }
        case TOKEN_INDEX_ONE:
        {
            appendUInt32ToString(&result, compiler->index + 1);
            break;
        }
        case TOKEN_COMM_INTERP: /* FALLTHROUGH */
        case TOKEN_COMMAND: appendToString(&result, token->start, token->length); break;
        default: errorAtCurrent(compiler, "Malformed command."); goto fail;
        }
        if (checkCompiler(compiler, TOKEN_COMMAND)) break;
        advanceCompiler(compiler);
    }

    *dest = result.string;
    disownString(&result);
    consume(compiler, TOKEN_COMMAND, "Expected command.");
    return;

fail:
    freeString(&result);
    return;
}

static void
compileHint(Compiler* compiler, KeyChord* keyChord)
{
    assert(compiler && keyChord);
    if (compiler->panicMode) return;

    String result = {0};
    initString(&result);
    appendToString(&result, keyChord->key, strlen(keyChord->key));
    appendToString(&result, delimiter, delimiterLen);
    appendToString(&result, keyChord->description, strlen(keyChord->description));
    keyChord->hint = result.string;
    disownString(&result);
}

static bool
compileHook(Compiler* compiler, KeyChord* keyChord, TokenType type)
{
    assert(compiler && keyChord);
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
        compileCommand(compiler, keyChord, &keyChord->before);
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
        compileCommand(compiler, keyChord, &keyChord->after);
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
        compileCommand(compiler, keyChord, &keyChord->before);
        keyChord->flags.syncBefore = true;
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
        compileCommand(compiler, keyChord, &keyChord->after);
        keyChord->flags.syncAfter = true;
        return true;
    }
    default: return false;
    }
}

static bool
compileFlag(Compiler* compiler, KeyChord* keyChord, TokenType type)
{
    assert(compiler && keyChord);
    if (compiler->panicMode) return false;

    switch (type)
    {
    case TOKEN_KEEP: keyChord->flags.keep = true; return true;
    case TOKEN_CLOSE: keyChord->flags.close = true; return true;
    case TOKEN_INHERIT: keyChord->flags.inherit = true; return true;
    case TOKEN_IGNORE: keyChord->flags.ignore = true; return true;
    case TOKEN_UNHOOK: keyChord->flags.unhook = true; return true;
    case TOKEN_DEFLAG: keyChord->flags.deflag = true; return true;
    case TOKEN_NO_BEFORE: keyChord->flags.nobefore = true; return true;
    case TOKEN_NO_AFTER: keyChord->flags.noafter = true; return true;
    case TOKEN_WRITE: keyChord->flags.write = true; return true;
    case TOKEN_SYNC_CMD: keyChord->flags.syncCommand = true; return true;
    default: return false;
    }
}

static void
compileHooksAndFlags(Compiler* compiler, KeyChord* keyChord)
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

    KeyChord* keyChord = &compiler->keyChord;
    initKeyChord(keyChord);

    compileMods(compiler, keyChord);
    compileKey(compiler, keyChord);
    compileDescription(compiler, keyChord);
    compileHint(compiler, keyChord);
    compileHooksAndFlags(compiler, keyChord);

    /* Prefix */
    if (checkCompiler(compiler, TOKEN_LEFT_BRACE)) return;

    compileCommand(compiler, keyChord, &keyChord->command);

    /* Check for brace after command */
    if (checkCompiler(compiler, TOKEN_LEFT_BRACE))
    {
        errorAtCurrent(compiler, "Expected end of key chord after command but got '{'.");
        return;
    }

    writeKeyChordArray(compiler->keyChordDest, keyChord);
    initKeyChord(keyChord);

    /* Increment index counter */
    compiler->index++;
}

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
        case TOKEN_THIS_KEY: appendToString(&result, to->key, strlen(to->key)); break;
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

    *dest = result.string;
    disownString(&result);
}

static void
compileString(char* from, char** to)
{
    assert(from && to);

    String result = {0};
    initString(&result);
    appendToString(&result, from, strlen(from));
    *to = result.string;
    disownString(&result);
}

static void
compileMissingKeyChordInfo(
    Compiler* compiler,
    KeyChord* from,
    KeyChord* to,
    TokenArray* descriptionTokens,
    TokenArray* commandTokens,
    size_t index)
{
    assert(compiler && from && to && descriptionTokens && commandTokens);

    if (!to->description) compileStringFromTokens(descriptionTokens, to, &to->description, index);
    if (!to->hint) compileHint(compiler, to);
    if (!to->command) compileStringFromTokens(commandTokens, to, &to->command, index);
    if (!to->before) compileString(from->before, &to->before);
    if (!to->after) compileString(from->after, &to->after);
    if (keyChordHasDefaultFlags(&to->flags)) copyKeyChordFlags(&from->flags, &to->flags);
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

    size_t arrayStart = compiler->keyChordDest->count;

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

        KeyChord* keyChord = &compiler->keyChord;
        initKeyChord(keyChord);

        if (matchCompiler(compiler, TOKEN_LEFT_PAREN))
        {
            compileMods(compiler, keyChord);
            compileKey(compiler, keyChord);
            compileDescription(compiler, keyChord);
            compileHint(compiler, keyChord);
            compileHooksAndFlags(compiler, keyChord);
            compileCommand(compiler, keyChord, &keyChord->command);
            consume(compiler, TOKEN_RIGHT_PAREN, "Expect closing parenthesis after '('.");
        }
        else
        {
            compileMods(compiler, keyChord);
            compileKey(compiler, keyChord);
        }

        writeKeyChordArray(compiler->keyChordDest, keyChord);

        /* increment index counter */
        compiler->index++;
    }

    consume(compiler, TOKEN_RIGHT_BRACKET, "Expect ']' after key list.");
    KeyChord dummy = {0};
    initKeyChord(&dummy);

    TokenArray descriptionTokens = {0};
    initTokenArray(&descriptionTokens);
    TokenArray commandTokens = {0};
    initTokenArray(&commandTokens);

    collectDescriptionTokens(compiler, &descriptionTokens);
    compileHooksAndFlags(compiler, &dummy);
    collectCommandTokens(compiler, &commandTokens);

    /* Write chords in chord array to destination */
    KeyChordArray* array = compiler->keyChordDest;
    size_t arrayEnd = array->count;
    for (size_t i = arrayStart; i < arrayEnd; i++)
    {
        KeyChord* keyChord = &(*array->keyChords)[i];
        compileMissingKeyChordInfo(
            compiler, &dummy, keyChord, &descriptionTokens, &commandTokens, i - arrayStart
        );
    }

    return;

fail:
    compiler->hadError = true;
    return;
}

static void
compileNullKeyChord(Compiler* compiler)
{
    assert(compiler);

    writeKeyChordArray(compiler->keyChordDest, &nullKeyChord);
}

static void
setBeforeHook(KeyChord* parent, KeyChord* child)
{
    assert(parent && child);

    /* Children that opt out do not inherit */
    if (child->flags.nobefore || !parent->before) return;

    String result = {0};
    initString(&result);
    appendToString(&result, parent->before, strlen(parent->before));
    child->before = result.string;
    disownString(&result);
}

static void
setAfterHook(KeyChord* parent, KeyChord* child)
{
    assert(parent && child);

    /* Children that opt out do not inherit */
    if (child->flags.noafter || !parent->after) return;

    String result = {0};
    initString(&result);
    appendToString(&result, parent->after, strlen(parent->after));
    child->before = result.string;
    disownString(&result);
}

static void
setHooks(KeyChord* parent, KeyChord* child)
{
    assert(parent && child);

    /* Children that opt out do not inherit */
    if (child->flags.unhook) return;

    setBeforeHook(parent, child);
    setAfterHook(parent, child);
}

static void
setFlags(KeyChordFlags* parent, KeyChordFlags* child)
{
    assert(parent && child);

    /* Children that opt out do not inherit */
    if (child->deflag) return;

    if (parent->keep && !child->close) child->keep = parent->keep;
    if (parent->write && !child->execute) child->write = parent->execute;
    if (parent->syncCommand) child->syncCommand = parent->syncCommand;
    if (parent->syncBefore) child->syncBefore = parent->syncBefore;
    if (parent->syncAfter) child->syncAfter = parent->syncAfter;
}

static void
setHooksAndFlags(KeyChord* parent, KeyChordArray* children)
{
    assert(parent && children);

    for (size_t i = 0; i < children->count; i++)
    {
        KeyChord* child = &(*children->keyChords)[i];
        /* Don't make any changes to defiant children */
        if (child->flags.ignore) continue;

        /* Prefixes don't inherit unless requested */
        if (child->keyChords && !child->flags.inherit) continue;

        setHooks(parent, child);
        setFlags(&parent->flags, &child->flags);
        if (child->keyChords)
        {
            KeyChordArray array = {0};
            makePsuedoKeyChordArray(&array, &child->keyChords);
            setHooksAndFlags(child, &array);
        }
    }
}

static void
compilePrefix(Compiler* compiler)
{
    assert(compiler);

    KeyChord* keyChord = &compiler->keyChord;

    /* Backup information */
    KeyChordArray* parent = compiler->keyChordPrefix;
    KeyChordArray* dest = compiler->keyChordDest;
    KeyChord* source = keyChord->keyChords;
    uint32_t outerIndex = compiler->index;
    compiler->index = 0;

    /* advance */
    compiler->keyChordPrefix = compiler->keyChordDest;
    writeKeyChordArray(compiler->keyChordDest, keyChord);

    /* start new scope */
    initKeyChordArray(compiler->keyChordDest, &source);

    /* Compile children */
    while (!compilerIsAtEnd(compiler) && !checkCompiler(compiler, TOKEN_RIGHT_BRACE))
    {
        compileKeyChord(compiler);
    }
    consume(compiler, TOKEN_RIGHT_BRACE, "Expect '}' after prefix.");


    /* end prefix */
    setHooksAndFlags(keyChord, compiler->keyChordDest);
    compileNullKeyChord(compiler);
    compiler->keyChordPrefix = parent;
    compiler->keyChordDest = dest;
    compiler->index = outerIndex;
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

KeyChord*
compileKeyChords(Compiler* compiler, Menu* menu)
{
    assert(compiler && menu);

    /* Initialize globals */
    initKeyChord(&nullKeyChord);
    nullKeyChord.state =KEY_CHORD_STATE_IS_NULL;
    debug = menu->debug;
    delimiter = menu->delimiter;
    delimiterLen = strlen(delimiter);

    KeyChord* keyChords = &menu->keyChordsHead;
    initKeyChordArray(compiler->keyChordDest, &keyChords);
    if (debug) debugPrintScannedTokenHeader();

    advanceCompiler(compiler);
    while (!compilerIsAtEnd(compiler))
    {
        compileKeyChord(compiler);
    }

    compileNullKeyChord(compiler);

    if (debug) debugPrintScannedTokenFooter();

    return compiler->hadError ? NULL : *compiler->keyChords.keyChords;
}

void
initCompiler(Compiler* compiler, char *source, const char *filepath)
{
    assert(compiler && source && filepath);

    initScanner(&compiler->scanner, source, filepath);
    compiler->hadError = false;
    compiler->panicMode = false;
    compiler->index = 0;
    initKeyChord(&compiler->keyChord);
    compiler->keyChordDest = &compiler->keyChords;
    compiler->keyChordPrefix = NULL;
    compiler->source = source;
}
