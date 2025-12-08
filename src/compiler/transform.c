#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* common includes */
#include "common/key_chord.h"
#include "common/property.h"
#include "common/span.h"
#include "common/stack.h"
#include "common/vector.h"

/* local includes */
#include "resolve.h"
#include "transform.h"

typedef enum
{
    KEY_CAT_SPECIAL,
    KEY_CAT_NUMBER,
    KEY_CAT_SYMBOL,
    KEY_CAT_LETTER,
} KeyCategory;

static inline char
getFirstChar(const String* repr)
{
    if (!repr || !repr->data || repr->length == 0) return '\0';
    return repr->data[0];
}

static inline char
toLower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

static inline KeyCategory
getKeyCategory(char c, SpecialKey special)
{
    if (special != SPECIAL_KEY_NONE) return KEY_CAT_SPECIAL;
    if (c >= '0' && c <= '9') return KEY_CAT_NUMBER;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return KEY_CAT_LETTER;
    return KEY_CAT_SYMBOL;
}

static int
compareKeyChords(const void* a, const void* b)
{
    const KeyChord* ca = a;
    const KeyChord* cb = b;
    const Key*      ka = &ca->key;
    const Key*      kb = &cb->key;

    char        ac   = getFirstChar(&ka->repr);
    char        bc   = getFirstChar(&kb->repr);
    KeyCategory aCat = getKeyCategory(ac, ka->special);
    KeyCategory bCat = getKeyCategory(bc, kb->special);

    if (aCat != bCat) return (int)aCat - (int)bCat;

    bool aMods = ka->mods != MOD_NONE;
    bool bMods = kb->mods != MOD_NONE;
    if (aMods != bMods) return aMods ? 1 : -1;

    if (aCat == KEY_CAT_LETTER)
    {
        char aLower = toLower(ac);
        char bLower = toLower(bc);
        if (aLower != bLower) return aLower - bLower;

        bool aIsLower = (ac >= 'a' && ac <= 'z');
        bool bIsLower = (bc >= 'a' && bc <= 'z');
        if (aIsLower != bIsLower) return aIsLower ? -1 : 1;
    }

    /* Compare String values lexicographically */
    size_t minLen = (ka->repr.length < kb->repr.length) ? ka->repr.length : kb->repr.length;
    int    cmp    = memcmp(ka->repr.data, kb->repr.data, minLen);
    if (cmp != 0) return cmp;
    return (ka->repr.length < kb->repr.length) ? -1 : (ka->repr.length > kb->repr.length) ? 1
                                                                                          : 0;
}

void
keyChordSpanSort(Span* chords)
{
    assert(chords);

    /* Recurse into children first */
    spanForEach(chords, KeyChord, chord)
    {
        if (chord->keyChords.count != 0)
        {
            keyChordSpanSort(&chord->keyChords);
        }
    }

    if (chords->count < 2) return;

    qsort(chords->data, chords->count, sizeof(KeyChord), compareKeyChords);
}

static void
keyChordVectorSort(Vector* chords)
{
    assert(chords);

    /* Root is Vector, children are Span */
    vectorForEach(chords, KeyChord, chord)
    {
        if (chord->keyChords.count != 0)
        {
            keyChordSpanSort(&chord->keyChords);
        }
    }

    size_t n = vectorLength(chords);
    if (n < 2) return;

    qsort(VECTOR_AS(chords, KeyChord), n, sizeof(KeyChord), compareKeyChords);
}

static void
copyPropertyIfChildEmpty(const KeyChord* parent, KeyChord* child, PropId id)
{
    assert(parent), assert(child);

    const Property* parentProp = propGetConst(parent, id);
    Property*       childProp  = propGet(child, id);

    if (!propertyHasContent(parentProp)) return;
    if (propertyHasContent(childProp)) return;

    propertyFree(childProp);
    propertyCopy(parentProp, childProp);
}

static void
setBeforeHook(KeyChord* parent, KeyChord* child)
{
    assert(parent), assert(child);

    if (chordFlagIsActive(child->flags, FLAG_NO_BEFORE)) return;

    const Property* parentBefore = propGetConst(parent, KC_PROP_BEFORE);
    if (!propertyHasContent(parentBefore)) return;

    copyPropertyIfChildEmpty(parent, child, KC_PROP_BEFORE);

    if (chordFlagIsActive(parent->flags, FLAG_SYNC_BEFORE))
        child->flags |= FLAG_SYNC_BEFORE;
}

static void
setAfterHook(KeyChord* parent, KeyChord* child)
{
    assert(parent), assert(child);

    if (chordFlagIsActive(child->flags, FLAG_NO_AFTER)) return;

    const Property* parentAfter = propGetConst(parent, KC_PROP_AFTER);
    if (!propertyHasContent(parentAfter)) return;

    copyPropertyIfChildEmpty(parent, child, KC_PROP_AFTER);

    if (chordFlagIsActive(parent->flags, FLAG_SYNC_AFTER))
        child->flags |= FLAG_SYNC_AFTER;
}

static void
setHooks(KeyChord* parent, KeyChord* child)
{
    assert(parent), assert(child);

    if (chordFlagIsActive(child->flags, FLAG_UNHOOK)) return;

    setBeforeHook(parent, child);
    setAfterHook(parent, child);
}

static ChordFlag
setFlags(ChordFlag parent, ChordFlag child)
{
    if (chordFlagIsActive(child, FLAG_DEFLAG)) return child;

    if (!chordFlagIsActive(child, FLAG_CLOSE) && chordFlagIsActive(parent, FLAG_KEEP))
    {
        child |= FLAG_KEEP;
    }
    if (!chordFlagIsActive(child, FLAG_EXECUTE) && chordFlagIsActive(parent, FLAG_WRITE))
    {
        child |= FLAG_WRITE;
    }
    if (chordFlagIsActive(parent, FLAG_SYNC_COMMAND))
    {
        child |= FLAG_SYNC_COMMAND;
    }

    return child;
}

static void
setHooksAndFlagsSpan(KeyChord* parent, Span* children)
{
    assert(parent), assert(children);

    spanForEach(children, KeyChord, child)
    {
        if (chordFlagIsActive(child->flags, FLAG_IGNORE)) continue;

        bool isPrefix      = child->keyChords.count != 0;
        bool shouldInherit = !isPrefix || chordFlagIsActive(child->flags, FLAG_INHERIT);

        if (shouldInherit)
        {
            setHooks(parent, child);
            child->flags = setFlags(parent->flags, child->flags);

            const Property* parentWrapCmd = propGetConst(parent, KC_PROP_WRAP_CMD);
            const Property* childWrapCmd  = propGetConst(child, KC_PROP_WRAP_CMD);
            if (!chordFlagIsActive(child->flags, FLAG_UNWRAP) &&
                !propertyHasContent(childWrapCmd) &&
                propertyHasContent(parentWrapCmd))
            {
                copyPropertyIfChildEmpty(parent, child, KC_PROP_WRAP_CMD);
            }

            const Property* childTitle  = propGetConst(child, KC_PROP_TITLE);
            const Property* parentTitle = propGetConst(parent, KC_PROP_TITLE);
            if (isPrefix &&
                !propertyHasContent(childTitle) &&
                propertyHasContent(parentTitle))
            {
                copyPropertyIfChildEmpty(parent, child, KC_PROP_TITLE);
            }
        }

        if (isPrefix) setHooksAndFlagsSpan(child, &child->keyChords);
    }
}

void
propagateInheritanceSpan(Span* chords)
{
    assert(chords);

    /* For top-level chords, only prefixes need inheritance propagation */
    spanForEach(chords, KeyChord, chord)
    {
        if (chord->keyChords.count != 0)
        {
            setHooksAndFlagsSpan(chord, &chord->keyChords);
        }
    }
}

static void
propagateInheritance(Vector* chords)
{
    assert(chords);

    /* Root is Vector, children are Span */
    vectorForEach(chords, KeyChord, chord)
    {
        if (chord->keyChords.count != 0)
        {
            setHooksAndFlagsSpan(chord, &chord->keyChords);
        }
    }
}

static int
compareSize_t(const void* a, const void* b)
{
    size_t x = *(const size_t*)a;
    size_t y = *(const size_t*)b;
    return (x > y) - (x < y);
}

static void freeKeyChordVectorProps(KeyChord* chord);

static void
freeKeyChordSpanPropsRecursive(Span* span)
{
    assert(span);

    spanForEach(span, KeyChord, chord) { freeKeyChordVectorProps(chord); }
}

static void
freeKeyChordVectorProps(KeyChord* chord)
{
    assert(chord);

    keyFree(&chord->key);

    for (size_t i = 0; i < KC_PROP_COUNT; i++)
    {
        propertyFree(&chord->props[i]);
    }

    freeKeyChordSpanPropsRecursive(&chord->keyChords);
}

void
deduplicateKeyChordVector(Vector* chords)
{
    assert(chords);

    if (vectorIsEmpty(chords)) return;

    Stack stack = STACK_INIT(size_t);

    vectorForRange(chords, KeyChord, outerChord, 0, vectorLength(chords) - 1)
    {
        VectorIterator* outerIter = &iter;
        size_t          swapIdx   = outerIter->index;
        vectorForEachFrom(chords, KeyChord, innerChord, outerIter->index + 1)
        {
            if (keyIsEqual(&outerChord->key, &innerChord->key))
            {
                stackPush(&stack, &iter.index);
                swapIdx = iter.index;
            }
        }

        vectorSwap(chords, outerIter->index, swapIdx);
    }

    vectorSort(&stack, compareSize_t);

    while (!stackIsEmpty(&stack))
    {
        size_t    index = *(STACK_PEEK(&stack, size_t));
        KeyChord* dup   = VECTOR_GET(chords, KeyChord, index);
        freeKeyChordVectorProps(dup);
        vectorRemove(chords, index);

        while (!stackIsEmpty(&stack) && index == *(STACK_PEEK(&stack, size_t)))
        {
            stackPop(&stack);
        }
    }

    stackFree(&stack);
}

bool
transform(Vector* chords, Menu* menu, Scanner* scanner)
{
    assert(chords), assert(menu), assert(scanner);

    deduplicateKeyChordVector(chords);
    propagateInheritance(chords);

    if (!resolve(chords, menu, scanner))
    {
        return false;
    }

    if (menu->sort) keyChordVectorSort(chords);

    return true;
}
