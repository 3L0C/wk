#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

/* common includes */
#include "common/arena.h"
#include "common/common.h"
#include "common/debug.h"
#include "common/key_chord.h"
#include "common/menu.h"
#include "common/property.h"
#include "common/span.h"
#include "common/string.h"
#include "common/vector.h"

/* local includes */
#include "compiler.h"
#include "debug.h"
#include "parser.h"
#include "preprocessor.h"
#include "scanner.h"
#include "token.h"
#include "transform.h"

void
compilerInitChord(KeyChord* chord)
{
    assert(chord);

    keyChordInit(chord);

    for (size_t i = 0; i < KC_PROP_COUNT; i++)
    {
        Property* prop = &chord->props[i];
        switch (i)
        {
        /* Properties that need token arrays during compilation */
        case KC_PROP_DESCRIPTION: /* FALLTHROUGH */
        case KC_PROP_COMMAND:
        case KC_PROP_BEFORE:
        case KC_PROP_AFTER:
        case KC_PROP_WRAP_CMD:
        case KC_PROP_TITLE:
        case KC_PROP_GOTO:
        {
            prop->type           = PROP_TYPE_ARRAY;
            prop->value.as_array = VECTOR_INIT(Token);
        }
        }
    }
}

void
compilerFreeChord(KeyChord* chord)
{
    assert(chord);

    keyFree(&chord->key);

    for (size_t i = 0; i < KC_PROP_COUNT; i++)
    {
        propertyFree(&chord->props[i]);
    }

    compilerFreeChordSpan(&chord->keyChords);
}

void
compilerFreeChordSpan(Span* span)
{
    assert(span);

    spanForEach(span, KeyChord, chord) { compilerFreeChord(chord); }
}

void
compilerFreeChordVector(Vector* vec)
{
    assert(vec);

    vectorForEach(vec, KeyChord, chord) { compilerFreeChord(chord); }

    vectorFree(vec);
}

Span*
compileKeyChords(Menu* menu, char* source, const char* filepath)
{
    assert(menu), assert(source), assert(filepath);

    Scanner scanner;
    scannerInit(&scanner, source, filepath);

    if (menu->debug) debugPrintScannedTokenHeader();

    Vector chords = parse(&scanner, menu);

    if (!transform(&chords, menu, &scanner))
    {
        return NULL;
    }

    menu->compiledKeyChords = SPAN_FROM_VECTOR(&menu->arena, &chords, KeyChord);
    menu->keyChords         = &menu->compiledKeyChords;

    if (menu->debug)
    {
        debugPrintScannedTokenFooter();
        if (menu->keyChords->count != 0)
        {
            disassembleKeyChordSpan(menu->keyChords, 0);
        }
    }

    return menu->keyChords;
}

Span*
compile(Menu* menu, const char* filepath)
{
    assert(menu);

    Arena compilerArena;
    arenaInit(&compilerArena);

    String      source;
    const char* effectivePath;

    if (filepath)
    {
        source        = readFileToArena(&compilerArena, filepath);
        effectivePath = filepath;
    }
    else
    {
        char* stdinData = ARENA_ADOPT_VECTOR(&compilerArena, &menu->client.script, char);
        source.data     = stdinData;
        source.length   = stdinData ? strlen(stdinData) : 0;
        effectivePath   = "<stdin>";
    }

    if (stringIsEmpty(&source))
    {
        arenaFree(&compilerArena);
        return NULL;
    }

    String processedSource = preprocessorRun(menu, source, effectivePath, &compilerArena);
    if (stringIsEmpty(&processedSource))
    {
        errorMsg("Failed while running preprocessor on `wks` file: '%s'.", effectivePath);
        arenaFree(&compilerArena);
        return NULL;
    }

    Span* result = compileKeyChords(menu, (char*)processedSource.data, effectivePath);

    arenaFree(&compilerArena);

    return result;
}
