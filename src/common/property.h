#ifndef WK_COMMON_PROPERTY_H_
#define WK_COMMON_PROPERTY_H_

#include <stdbool.h>
#include <stddef.h>

#include "array.h"
#include "property_def.h"
#include "string.h"

/* Forward declarations to avoid circular dependency */
typedef struct KeyChord KeyChord;

/* Property type enumeration */
typedef enum
{
    PROP_TYPE_NONE,
    PROP_TYPE_STRING,
    PROP_TYPE_INT,
    PROP_TYPE_BOOL,
    PROP_TYPE_ARRAY,
    PROP_TYPE_COLOR
} PropertyType;

/* Property ID enumeration - auto-generated from PROPERTY_LIST */
typedef enum
{
#define PROPERTY(id, field, accessor, typecat, ctype) id,
    PROPERTY_LIST
#undef PROPERTY
        PROP_COUNT /* Total number of properties */
} PropertyId;

/* Property value union - auto-generated from PROPERTY_LIST */
typedef union
{
#define PROPERTY(id, field, accessor, typecat, ctype) ctype as_##field;
    PROPERTY_LIST
#undef PROPERTY
} PropertyValue;

/* Property container */
typedef struct
{
    PropertyType  type;
    PropertyValue value;
} Property;

/* Property metadata for introspection */
typedef struct
{
    const char*  name;
    PropertyType type;
} PropertyInfo;

/* Property info table - defined in property.c */
extern const PropertyInfo PROPERTY_INFO_TABLE[PROP_COUNT];

/* Property initialization and cleanup */
void propertyInit(Property* prop);
void propertyFree(Property* prop);
void propertyCopy(const Property* from, Property* to, PropertyId id);

/* Property query functions */
bool propIsSet(const KeyChord* chord, PropertyId id);
bool propIsEmpty(const KeyChord* chord, PropertyId id);

/* Auto-generated getter/setter declarations */
#define PROPERTY(id, field, accessor, typecat, ctype)                 \
    ctype*       keyChordGet##accessor(KeyChord* chord);              \
    const ctype* keyChordGet##accessor##Const(const KeyChord* chord); \
    void         keyChordSet##accessor(KeyChord* chord, const ctype* value);

PROPERTY_LIST
#undef PROPERTY

#endif /* WK_COMMON_PROPERTY_H_ */
