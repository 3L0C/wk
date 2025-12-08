#ifndef WK_COMMON_KEY_CHORD_DEF_H_
#define WK_COMMON_KEY_CHORD_DEF_H_

/* X-Macro KeyChord Property Slot Registry
 *
 * This defines KeyChord's property SLOTS, not the Property types themselves.
 * Property types are defined in property_def.h.
 *
 * Format: KC_PROP(id, name, accessor)
 * - id: Enum value (KC_PROP_DESCRIPTION, etc.)
 * - name: Lowercase name for introspection ("description", etc.)
 * - accessor: CamelCase for function generation (keyChordDescription, etc.)
 */
#define KEY_CHORD_PROP_LIST                                \
    KC_PROP(KC_PROP_DESCRIPTION, description, Description) \
    KC_PROP(KC_PROP_COMMAND, command, Command)             \
    KC_PROP(KC_PROP_BEFORE, before, Before)                \
    KC_PROP(KC_PROP_AFTER, after, After)                   \
    KC_PROP(KC_PROP_WRAP_CMD, wrapCmd, WrapCmd)            \
    KC_PROP(KC_PROP_TITLE, title, Title)                   \
    KC_PROP(KC_PROP_GOTO, gotoPath, Goto)

#endif /* WK_COMMON_KEY_CHORD_DEF_H_ */
