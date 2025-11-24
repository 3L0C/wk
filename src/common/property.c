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
propertyInit(Property* prop, PropertyType type)
{
    assert(prop);

    prop->type = type;

    switch (type)
    {
    case PROP_TYPE_STRING:
        /* All string properties in the union share the same memory.
         * We use as_description as the canonical accessor for initialization. */
        prop->value.as_description = stringInit();
        break;
    case PROP_TYPE_INT:
    case PROP_TYPE_BOOL:
    case PROP_TYPE_COLOR:
        memset(&prop->value, 0, sizeof(PropertyValue));
        break;
    case PROP_TYPE_ARRAY:
        /* Would initialize Array type if we had array properties */
        break;
    default:
        prop->type = PROP_TYPE_NONE;
        break;
    }
}

void
propertyFree(Property* prop)
{
    assert(prop);

    switch (prop->type)
    {
    case PROP_TYPE_STRING:
        /* Free using canonical accessor */
        stringFree(&prop->value.as_description);
        break;
    case PROP_TYPE_ARRAY:
        /* Would free Array if we had array properties */
        break;
    default:
        break;
    }

    prop->type = PROP_TYPE_NONE;
}

void
propertyCopy(const Property* from, Property* to)
{
    assert(from), assert(to);

    to->type = from->type;

    switch (from->type)
    {
    case PROP_TYPE_STRING:
        /* Copy string value */
        to->value.as_description = from->value.as_description;
        break;
    default:
        /* For other types, copy the entire union */
        to->value = from->value;
        break;
    }
}

bool
propIsSet(const KeyChord* chord, PropertyId id)
{
    assert(chord);
    assert(id < PROP_COUNT);

    const Property* prop = &chord->props[id];

    switch (prop->type)
    {
    case PROP_TYPE_STRING:
        /* String properties are "set" if they're not empty */
        return !stringIsEmpty(&prop->value.as_description);
    case PROP_TYPE_NONE:
        return false;
    default:
        /* Other types are "set" if type is not NONE */
        return true;
    }
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
        chord->props[id].value.as_##field = *value;                  \
    }

PROPERTY_LIST
#undef PROPERTY
