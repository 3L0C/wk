#ifndef WK_COMMON_KEY_CHORD_H_
#define WK_COMMON_KEY_CHORD_H_

#include <stdbool.h>
#include <stddef.h>

/* common includes */
#include "chord_flag.h"
#include "key.h"
#include "key_chord_def.h"
#include "property.h"
#include "span.h"

typedef enum
{
#define KC_PROP(id, name, accessor) id,
    KEY_CHORD_PROP_LIST
#undef KC_PROP
        KC_PROP_COUNT
} PropId;

typedef struct KeyChord
{
    Key       key;
    Property  props[KC_PROP_COUNT];
    ChordFlag flags;
    Span      keyChords;
} KeyChord;

void            keyChordCopy(const KeyChord* from, KeyChord* to);
void            keyChordFree(KeyChord* keyChord);
void            keyChordInit(KeyChord* keyChord);
void            keyChordsFree(Span* keyChords);
Property*       propGet(KeyChord* chord, PropId id);
const Property* propGetConst(const KeyChord* chord, PropId id);
bool            propHasContent(const KeyChord* chord, PropId id);
void            propInitAsArray(KeyChord* chord, PropId id, size_t itemSize);
bool            propIsSet(const KeyChord* chord, PropId id);
const char*     propRepr(PropId id);

/* Type-specific slot accessors. Return NULL if prop->type doesn't match */
#define PROP_TYPE_X(name, ctype, accessor, field)            \
    ctype*       prop##accessor(KeyChord* chord, PropId id); \
    const ctype* prop##accessor##Const(const KeyChord* chord, PropId id);
PROPERTY_TYPE_LIST
#undef PROP_TYPE_X

#endif /* WK_COMMON_KEY_CHORD_H_ */
