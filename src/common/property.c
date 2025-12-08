#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "common/vector.h"
#include "property.h"
#include "string.h"

void
propertyClear(Property* prop)
{
    propertyFree(prop);
}

void
propertyCopy(const Property* from, Property* to)
{
    assert(from), assert(to);

    to->type = from->type;
    if (from->type == PROP_TYPE_NONE) return;

    switch (from->type)
    {
    case PROP_TYPE_ARRAY: to->value.as_array = vectorCopy(&from->value.as_array); break;
    case PROP_TYPE_STRING: to->value.as_string = from->value.as_string; break;
    case PROP_TYPE_INT: to->value.as_int = from->value.as_int; break;
    case PROP_TYPE_BOOL: to->value.as_bool = from->value.as_bool; break;
    case PROP_TYPE_COLOR: to->value.as_color = from->value.as_color; break;
    case PROP_TYPE_NONE: /* FALLTHROUGH */
    case PROP_TYPE_COUNT: break;
    }
}

void
propertyFree(Property* prop)
{
    assert(prop);

    switch (prop->type)
    {
    case PROP_TYPE_ARRAY: vectorFree(&prop->value.as_array); break;
    case PROP_TYPE_NONE: /* FALLTHROUGH */
    case PROP_TYPE_INT:
    case PROP_TYPE_BOOL:
    case PROP_TYPE_COLOR:
    case PROP_TYPE_COUNT:
    case PROP_TYPE_STRING: break;
    }

    prop->type = PROP_TYPE_NONE;
}

bool
propertyHasContent(const Property* prop)
{
    if (!prop) return false;
    switch (prop->type)
    {
    case PROP_TYPE_NONE: return false;
    case PROP_TYPE_ARRAY: return !vectorIsEmpty(&prop->value.as_array);
    case PROP_TYPE_STRING: return !stringIsEmpty(&prop->value.as_string);
    default: return true;
    }
}

void
propertyInit(Property* prop)
{
    assert(prop);
    prop->type = PROP_TYPE_NONE;
    memset(&prop->value, 0, sizeof(PropertyValue));
}

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
