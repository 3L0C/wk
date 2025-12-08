#ifndef WK_COMMON_PROPERTY_H_
#define WK_COMMON_PROPERTY_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "property_def.h"
#include "string.h"
#include "vector.h"

/* Property type enumeration generated from PROPERTY_TYPE_LIST */
typedef enum
{
    PROP_TYPE_NONE,
#define PROP_TYPE_X(name, ctype, accessor, field) PROP_TYPE_##name,
    PROPERTY_TYPE_LIST
#undef PROP_TYPE_X
        PROP_TYPE_COUNT
} PropertyType;

/* Property value union generated from PROPERTY_TYPE_LIST */
typedef union
{
#define PROP_TYPE_X(name, ctype, accessor, field) ctype field;
    PROPERTY_TYPE_LIST
#undef PROP_TYPE_X
} PropertyValue;

/* Property container a generic typed value */
typedef struct
{
    PropertyType  type;
    PropertyValue value;
} Property;

/* Direct value access macro use on Property* */
#define PROP_VAL(prop, field) (&(prop)->value.field)

#define PROP_TYPE_X(name, ctype, accessor, field)                              \
    static inline ctype* propertyAs##accessor(Property* prop)                  \
    {                                                                          \
        assert(prop);                                                          \
        return (prop->type != PROP_TYPE_##name) ? NULL : &(prop->value.field); \
    }
PROPERTY_TYPE_LIST
#undef PROP_TYPE_X

#define PROP_SET_TYPE(prop, name) ((prop)->type = PROP_TYPE_##name)

void propertyClear(Property* prop);
void propertyCopy(const Property* from, Property* to);
void propertyFree(Property* prop);
bool propertyHasContent(const Property* prop);
void propertyInit(Property* prop);
bool propertyIsSet(const Property* prop);
bool propertyIsType(const Property* prop, PropertyType type);

#endif /* WK_COMMON_PROPERTY_H_ */
