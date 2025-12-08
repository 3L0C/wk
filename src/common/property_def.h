#ifndef WK_COMMON_PROPERTY_DEF_H_
#define WK_COMMON_PROPERTY_DEF_H_

/* X-Macro Type Registry for Property System
 *
 * This defines the available property VALUE TYPES, not property instances.
 * KeyChord's property slots are defined separately in key_chord_def.h.
 *
 * Format: PROP_TYPE_X(NAME, CType, Accessor, union_field)
 * - NAME: Enum suffix (PROP_TYPE_NAME)
 * - CType: The C type for this property type
 * - Accessor: CamelCase name for accessor function generation
 * - union_field: Field name in PropertyValue union (as_field)
 *
 * NOTE: PROP_TYPE_NONE is defined manually, not via this list.
 */
#define PROPERTY_TYPE_LIST                         \
    PROP_TYPE_X(STRING, String, String, as_string) \
    PROP_TYPE_X(INT, int32_t, Int, as_int)         \
    PROP_TYPE_X(BOOL, bool, Bool, as_bool)         \
    PROP_TYPE_X(COLOR, uint32_t, Color, as_color)  \
    PROP_TYPE_X(ARRAY, Vector, Vector, as_array)

#endif /* WK_COMMON_PROPERTY_DEF_H_ */
