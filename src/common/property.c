#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "key_chord.h"
#include "property.h"
#include "string.h"

/* Property info table - auto-generated from PROPERTY_LIST */
const PropertyInfo PROPERTY_INFO_TABLE[PROP_COUNT] = {
#define PROPERTY(id, field, accessor, typecat, ctype) \
    [id] = { .name = #field, .type = PROP_TYPE_##typecat },
    PROPERTY_LIST
#undef PROPERTY
};

void
propertyInit(Property* prop)
{
    assert(prop);
    prop->type = PROP_TYPE_NONE;
    memset(&prop->value, 0, sizeof(PropertyValue));
}

void
propertyFree(Property* prop)
{
    assert(prop);

    switch (prop->type)
    {
    case PROP_TYPE_STRING:
        stringFree(&prop->value.as_description);
        break;
    default:
        break;
    }

    prop->type = PROP_TYPE_NONE;
}

/* Type-specific copy macros for X-macro expansion */
#define PROP_COPY_STRING(from, to, field) \
    (to)->value.as_##field = stringCopy(&(from)->value.as_##field)

#define PROP_COPY_INT(from, to, field) \
    (to)->value.as_##field = (from)->value.as_##field

#define PROP_COPY_BOOL(from, to, field) \
    (to)->value.as_##field = (from)->value.as_##field

#define PROP_COPY_COLOR(from, to, field) \
    (to)->value.as_##field = (from)->value.as_##field

#define PROP_COPY_ARRAY(from, to, field) \
    (to)->value.as_##field = (from)->value.as_##field

void
propertyCopy(const Property* from, Property* to, PropertyId id)
{
    assert(from), assert(to);
    assert(id < PROP_COUNT);

    to->type = from->type;
    if (from->type == PROP_TYPE_NONE) return;

    switch (id)
    {
#define PROPERTY(pid, field, accessor, typecat, ctype) \
    case pid: PROP_COPY_##typecat(from, to, field); break;
        PROPERTY_LIST
#undef PROPERTY
    default:
        break;
    }
}

bool
propIsSet(const KeyChord* chord, PropertyId id)
{
    assert(chord);
    assert(id < PROP_COUNT);

    return chord->props[id].type != PROP_TYPE_NONE;
}

bool
propIsEmpty(const KeyChord* chord, PropertyId id)
{
    return !propIsSet(chord, id);
}

/* Auto-generated getter/setter implementations */
#define PROPERTY(id, field, accessor, typecat, ctype)                \
    ctype* keyChordGet##accessor(KeyChord* chord)                    \
    {                                                                \
        assert(chord);                                               \
        return &chord->props[id].value.as_##field;                   \
    }                                                                \
    const ctype* keyChordGet##accessor##Const(const KeyChord* chord) \
    {                                                                \
        assert(chord);                                               \
        return &chord->props[id].value.as_##field;                   \
    }                                                                \
    void keyChordSet##accessor(KeyChord* chord, const ctype* value)  \
    {                                                                \
        assert(chord), assert(value);                                \
        chord->props[id].type             = PROP_TYPE_##typecat;     \
        chord->props[id].value.as_##field = *value;                  \
    }

PROPERTY_LIST
#undef PROPERTY
