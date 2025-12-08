#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* common includes */
#include "common/common.h"
#include "common/key_chord.h"
#include "common/property.h"
#include "common/span.h"
#include "common/string.h"
#include "common/vector.h"

/* local includes */
#include "lazy_string.h"
#include "resolve.h"
#include "scanner.h"
#include "token.h"

typedef struct
{
    Arena*   arena;
    Vector*  userVars;
    Scanner* scanner;
    Menu*    menu;
    bool     debug;
    bool     hadError;
} Resolver;

static void
resolverInit(Resolver* r, Menu* m, Scanner* scanner)
{
    assert(r), assert(m), assert(scanner);

    r->arena    = &m->arena;
    r->userVars = &m->userVars;
    r->scanner  = scanner;
    r->menu     = m;
    r->debug    = m->debug;
    r->hadError = false;
}

static bool
resolveInterpolationToken(Resolver* r, Token* token, KeyChord* chord, LazyString* dest, size_t index)
{
    switch (token->type)
    {
    case TOKEN_THIS_KEY:
        lazyStringAppend(dest, chord->key.repr.data, chord->key.repr.length);
        return true;

    case TOKEN_INDEX:
        lazyStringAppendUInt32(r->arena, dest, index);
        return true;

    case TOKEN_INDEX_ONE:
        lazyStringAppendUInt32(r->arena, dest, index + 1);
        return true;

    case TOKEN_USER_VAR:
    {
        vectorForEach(r->userVars, const UserVar, var)
        {
            if (strncmp(var->key, token->start, token->length) == 0)
            {
                lazyStringAppendCString(dest, var->value);
                return true;
            }
        }
        scannerErrorAt(
            r->scanner,
            token,
            "Undefined variable '%%(%.*s)'. Use :var \"%.*s\" \"value\" to define it.",
            token->length,
            token->start,
            token->length,
            token->start);
        r->hadError = true;
        return true;
    }

    case TOKEN_WRAP_CMD_INTERP:
        if (r->menu->wrapCmd && r->menu->wrapCmd[0])
        {
            lazyStringAppendCString(dest, r->menu->wrapCmd);
        }
        return true;

    default:
        return false;
    }
}

static void
appendDescriptionWithCase(Resolver* r, LazyString* dest, TokenType type, const String* desc)
{
    assert(r), assert(dest), assert(desc);

    LazyStringCase state;
    switch (type)
    {
    case TOKEN_THIS_DESC_UPPER_FIRST: state = LAZY_STRING_CASE_UPPER_FIRST; break;
    case TOKEN_THIS_DESC_LOWER_FIRST: state = LAZY_STRING_CASE_LOWER_FIRST; break;
    case TOKEN_THIS_DESC_UPPER_ALL: state = LAZY_STRING_CASE_UPPER_ALL; break;
    case TOKEN_THIS_DESC_LOWER_ALL: state = LAZY_STRING_CASE_LOWER_ALL; break;
    default:
        errorMsg("Got unexpected token type to `appendDescriptionWithCase`.");
        lazyStringAppendCString(dest, desc->data);
        return;
    }

    lazyStringAppendWithState(r->arena, dest, desc->data, desc->length, state);
}

static void
resolveToken(Resolver* r, Token* token, KeyChord* chord, LazyString* dest, size_t index)
{
    assert(r), assert(token), assert(chord), assert(dest);

    if (resolveInterpolationToken(r, token, chord, dest, index)) return;

    switch (token->type)
    {
    case TOKEN_THIS_DESC:
    {
        const String* desc = propStringConst(chord, KC_PROP_DESCRIPTION);
        if (!stringIsEmpty(desc)) lazyStringAppendCString(dest, desc->data);
        break;
    }

    case TOKEN_THIS_DESC_UPPER_FIRST: /* FALLTHROUGH */
    case TOKEN_THIS_DESC_LOWER_FIRST:
    case TOKEN_THIS_DESC_UPPER_ALL:
    case TOKEN_THIS_DESC_LOWER_ALL:
    {
        const String* desc = propStringConst(chord, KC_PROP_DESCRIPTION);
        if (!stringIsEmpty(desc)) appendDescriptionWithCase(r, dest, token->type, desc);
        break;
    }

    case TOKEN_DESC_INTERP: /* FALLTHROUGH */
    case TOKEN_DESCRIPTION:
        lazyStringAppendEscString(dest, token->start, token->length);
        break;

    case TOKEN_COMM_INTERP: /* FALLTHROUGH */
    case TOKEN_COMMAND:
        lazyStringAppend(dest, token->start, token->length);
        break;

    default:
        errorMsg(
            "Got unexpected token when resolving token array: '%s'.",
            tokenLiteral(token->type));
        break;
    }
}

static void
resolveTokenVector(Resolver* r, Vector* tokens, KeyChord* chord, LazyString* dest, size_t index)
{
    assert(r), assert(tokens), assert(chord), assert(dest);

    vectorForEach(tokens, Token, token)
    {
        resolveToken(r, token, chord, dest, index);
    }

    lazyStringRtrim(dest);
}

static bool
isSingleSimpleToken(const Vector* tokens)
{
    if (tokens->length != 1) return false;
    Token* token = VECTOR_GET(tokens, Token, 0);
    return token->type == TOKEN_DESCRIPTION || token->type == TOKEN_COMMAND;
}

static void
resolveChordProperties(Resolver* r, KeyChord* chord, size_t index)
{
    assert(r), assert(chord);

    for (size_t i = 0; i < KC_PROP_COUNT; i++)
    {
        Property* prop = propGet(chord, (PropId)i);
        if (prop->type == PROP_TYPE_ARRAY)
        {
            Vector tokens = prop->value.as_array;

            if (!vectorIsEmpty(&tokens))
            {
                String result;

                if (isSingleSimpleToken(&tokens))
                {
                    Token* token = VECTOR_GET(&tokens, Token, 0);
                    result       = stringMake(r->arena, token->start, token->length);
                }
                else
                {
                    LazyString str = lazyStringInit();
                    resolveTokenVector(r, &tokens, chord, &str, index);
                    result = lazyStringToString(r->arena, &str);
                    lazyStringFree(&str);
                }

                vectorFree(&tokens);
                prop->value.as_string = result;
                prop->type            = PROP_TYPE_STRING;
            }
            else
            {
                vectorFree(&tokens);
                prop->type = PROP_TYPE_NONE;
            }
        }
    }
}

static void
resolveChordSpan(Resolver* r, Span* chords)
{
    assert(r), assert(chords);

    size_t index = 0;
    spanForEach(chords, KeyChord, chord)
    {
        resolveChordProperties(r, chord, index++);

        if (chord->keyChords.count != 0)
        {
            resolveChordSpan(r, &chord->keyChords);
        }
    }
}

static void
resolveChordVector(Resolver* r, Vector* chords)
{
    assert(r), assert(chords);

    vectorForEach(chords, KeyChord, chord)
    {
        resolveChordProperties(r, chord, iter.index);

        if (chord->keyChords.count != 0)
        {
            resolveChordSpan(r, &chord->keyChords);
        }
    }
}

bool
resolve(Vector* chords, Menu* menu, Scanner* scanner)
{
    assert(chords), assert(menu), assert(scanner);

    Resolver r;
    resolverInit(&r, menu, scanner);
    resolveChordVector(&r, chords);
    return !r.hadError;
}
