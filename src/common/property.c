#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "common/array.h"
#include "property.h"
#include "string.h"

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
    case PROP_TYPE_STRING: stringFree(&prop->value.as_string); break;
    case PROP_TYPE_ARRAY: arrayFree(&prop->value.as_array); break;
    case PROP_TYPE_NONE: /* FALLTHROUGH */
    case PROP_TYPE_INT:
    case PROP_TYPE_BOOL:
    case PROP_TYPE_COLOR:
    case PROP_TYPE_COUNT: break;
    }

    prop->type = PROP_TYPE_NONE;
}

void
propertyCopy(const Property* from, Property* to)
{
    assert(from), assert(to);

    to->type = from->type;
    if (from->type == PROP_TYPE_NONE) return;

    switch (from->type)
    {
    case PROP_TYPE_STRING: to->value.as_string = stringCopy(&from->value.as_string); break;
    case PROP_TYPE_ARRAY: to->value.as_array = arrayCopy(&from->value.as_array); break;
    case PROP_TYPE_INT: to->value.as_int = from->value.as_int; break;
    case PROP_TYPE_BOOL: to->value.as_bool = from->value.as_bool; break;
    case PROP_TYPE_COLOR: to->value.as_color = from->value.as_color; break;
    case PROP_TYPE_NONE:
    case PROP_TYPE_COUNT:
        break;
    }
}

void
propertyClear(Property* prop)
{
    propertyFree(prop);
}

/* Query operations */

bool
propertyIsSet(const Property* prop)
{
    assert(prop);
    return prop->type != PROP_TYPE_NONE;
}

bool
propertyIsType(const Property* prop, PropertyType type)
{
    assert(prop);
    return prop->type == type;
}
