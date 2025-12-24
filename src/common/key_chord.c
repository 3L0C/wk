#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

/* local includes */
#include "key_chord.h"
#include "property.h"
#include "vector.h"

static const char* keyChordPropNames[KC_PROP_COUNT] = {
#define KC_PROP(id, name, accessor) [id] = #id,
    KEY_CHORD_PROP_LIST
#undef KC_PROP
};

void
keyChordCopy(const KeyChord* from, KeyChord* to)
{
    assert(from), assert(to);

    keyCopy(&from->key, &to->key);

    for (size_t i = 0; i < KC_PROP_COUNT; i++)
    {
        propertyCopy(&from->props[i], &to->props[i]);
    }

    to->flags     = from->flags;
    to->keyChords = from->keyChords;
}

void
keyChordFree(KeyChord* keyChord)
{
    assert(keyChord);

    for (size_t i = 0; i < KC_PROP_COUNT; i++)
    {
        propertyFree(&keyChord->props[i]);
    }

    keyChordsFree(&keyChord->keyChords);
}

void
keyChordInit(KeyChord* keyChord)
{
    assert(keyChord);

    keyInit(&keyChord->key);

    for (size_t i = 0; i < KC_PROP_COUNT; i++)
    {
        propertyInit(&keyChord->props[i]);
    }

    keyChord->flags     = chordFlagInit();
    keyChord->keyChords = SPAN_EMPTY;
}

void
keyChordsFree(Span* keyChords)
{
    assert(keyChords);

    spanForEach(keyChords, KeyChord, keyChord)
    {
        keyChordFree(keyChord);
    }
}

Property*
propGet(KeyChord* chord, PropId id)
{
    assert(chord), assert(id < KC_PROP_COUNT);
    return &chord->props[id];
}

const Property*
propGetConst(const KeyChord* chord, PropId id)
{
    assert(chord), assert(id < KC_PROP_COUNT);
    return &chord->props[id];
}

bool
propHasContent(const KeyChord* chord, PropId id)
{
    assert(chord), assert(id < KC_PROP_COUNT);
    return propertyHasContent(propGetConst(chord, id));
}

void
propInitAsArray(KeyChord* chord, PropId id, size_t itemSize)
{
    assert(chord), assert(id < KC_PROP_COUNT);

    Property* prop       = &chord->props[id];
    prop->type           = PROP_TYPE_ARRAY;
    prop->value.as_array = vectorInit(itemSize);
}

bool
propIsSet(const KeyChord* chord, PropId id)
{
    assert(chord), assert(id < KC_PROP_COUNT);
    return propertyIsSet(&chord->props[id]);
}

const char*
propRepr(PropId id)
{
    assert(id < KC_PROP_COUNT);
    return keyChordPropNames[id];
}

#define PROP_TYPE_X(name, ctype, accessor, field)                        \
    ctype* prop##accessor(KeyChord* chord, PropId id)                    \
    {                                                                    \
        assert(chord);                                                   \
        Property* prop = propGet(chord, id);                             \
        if (prop->type != PROP_TYPE_##name) return NULL;                 \
        return &prop->value.field;                                       \
    }                                                                    \
    const ctype* prop##accessor##Const(const KeyChord* chord, PropId id) \
    {                                                                    \
        assert(chord);                                                   \
        const Property* prop = propGetConst(chord, id);                  \
        if (prop->type != PROP_TYPE_##name) return NULL;                 \
        return &prop->value.field;                                       \
    }
PROPERTY_TYPE_LIST

#undef PROP_TYPE_X
