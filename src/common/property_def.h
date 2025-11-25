#ifndef WK_COMMON_PROPERTY_DEF_H_
#define WK_COMMON_PROPERTY_DEF_H_

/* X-Macro Property Registry
 *
 * NOTE: do not use 'src/common/key_chord.h' types to avoid circular dependencies.
 *
 * Format: PROPERTY(ENUM_ID, fieldName, AccessorName, TYPE_CATEGORY, CType)
 */
#define PROPERTY_LIST                                                    \
    PROPERTY(PROP_DESCRIPTION, description, Description, STRING, String) \
    PROPERTY(PROP_COMMAND, command, Command, STRING, String)             \
    PROPERTY(PROP_BEFORE, before, Before, STRING, String)                \
    PROPERTY(PROP_AFTER, after, After, STRING, String)                   \
    PROPERTY(PROP_WRAP_CMD, wrapCmd, WrapCmd, STRING, String)            \
    PROPERTY(PROP_TITLE, title, Title, STRING, String)                   \
    PROPERTY(PROP_GOTO, gotoPath, Goto, STRING, String)

#endif /* WK_COMMON_PROPERTY_DEF_H_ */
