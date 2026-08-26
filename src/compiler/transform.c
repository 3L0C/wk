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
compareStringValues(const String* a, const String* b)
{
    size_t minLen = (a->length < b->length) ? a->length : b->length;
    int    cmp    = memcmp(a->data, b->data, minLen);
    if (cmp != 0) return cmp;

    if (a->length < b->length) return -1;
    if (a->length > b->length) return 1;
    return 0;
}

static int
compareGroupNames(const KeyChord* a, const KeyChord* b)
{
    const String* ga = propStringConst(a, KC_PROP_GROUP);
    const String* gb = propStringConst(b, KC_PROP_GROUP);

    bool aHas = ga && !stringIsEmpty(ga);
    bool bHas = gb && !stringIsEmpty(gb);

    if (!aHas && !bHas) return 0;
    if (aHas != bHas) return aHas ? 1 : -1;

    return compareStringValues(ga, gb);
}

static int
compareGroupColumns(const KeyChord* a, const KeyChord* b)
{
    const int32_t* ca = propIntConst(a, KC_PROP_GROUP_COLUMN);
    const int32_t* cb = propIntConst(b, KC_PROP_GROUP_COLUMN);

    int32_t va = ca ? *ca : -1;
    int32_t vb = cb ? *cb : -1;

    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

static int
compareKeyChords(const void* a, const void* b)
{
    const KeyChord* ca = a;
    const KeyChord* cb = b;

    int columnCmp = compareGroupColumns(ca, cb);
    if (columnCmp != 0) return columnCmp;

    int groupCmp = compareGroupNames(ca, cb);
    if (groupCmp != 0) return groupCmp;

    const Key* ka = &ca->key;
    const Key* kb = &cb->key;

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

    if (ka->repr.length < kb->repr.length) return -1;
    else if (ka->repr.length > kb->repr.length) return 1;
    else return 0;
}

/* Sorted columns keep alphabetical order: rewrite each scope's column
 * ordinals to the rank of the column's alphabetically-first group name. */
typedef struct
{
    int32_t       ordinal;
    const String* name;
} ColumnRank;

static int
compareColumnRanks(const void* a, const void* b)
{
    const ColumnRank* ra = a;
    const ColumnRank* rb = b;

    int cmp = compareStringValues(ra->name, rb->name);
    if (cmp != 0) return cmp;

    if (ra->ordinal < rb->ordinal) return -1;
    if (ra->ordinal > rb->ordinal) return 1;
    return 0;
}

static ColumnRank*
findColumnRank(Vector* ranks, int32_t ordinal)
{
    assert(ranks);

    vectorForEach(ranks, ColumnRank, rank)
    {
        if (rank->ordinal == ordinal) return rank;
    }
    return NULL;
}

/* Columns that share a first group name merge into one column, so sorted
 * mode keeps the pre-packing behavior: same-named groups coalesce. */
static size_t
firstRankIndexForName(const Vector* ranks, const String* name)
{
    assert(ranks), assert(name);

    vectorForEach(ranks, const ColumnRank, rank)
    {
        if (compareStringValues(rank->name, name) == 0) return iter.index;
    }
    return 0;
}

static void
remapColumnOrdinals(KeyChord* chords, size_t count)
{
    assert(chords);

    /* ponytail: linear rank lookups, fine for tens of columns */
    Vector ranks = VECTOR_INIT(ColumnRank);

    for (size_t i = 0; i < count; i++)
    {
        const int32_t* column = propIntConst(&chords[i], KC_PROP_GROUP_COLUMN);
        const String*  name   = propStringConst(&chords[i], KC_PROP_GROUP);
        if (!column || !name) continue;

        ColumnRank* found = findColumnRank(&ranks, *column);
        if (!found)
        {
            ColumnRank rank = { .ordinal = *column, .name = name };
            vectorAppend(&ranks, &rank);
        }
        else if (compareStringValues(name, found->name) < 0)
        {
            found->name = name;
        }
    }

    if (!vectorIsEmpty(&ranks))
    {
        qsort(
            VECTOR_AS(&ranks, ColumnRank),
            vectorLength(&ranks),
            sizeof(ColumnRank),
            compareColumnRanks);

        for (size_t i = 0; i < count; i++)
        {
            int32_t* column = propInt(&chords[i], KC_PROP_GROUP_COLUMN);
            if (!column) continue;

            ColumnRank* found = findColumnRank(&ranks, *column);
            if (found) *column = (int32_t)firstRankIndexForName(&ranks, found->name);
        }
    }

    vectorFree(&ranks);
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

    remapColumnOrdinals(chords->data, chords->count);
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

    remapColumnOrdinals(VECTOR_AS(chords, KeyChord), n);
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

    if (chordFlagIsActive(parent->flags, FLAG_SYNC_BEFORE)) child->flags |= FLAG_SYNC_BEFORE;
}

static void
setAfterHook(KeyChord* parent, KeyChord* child)
{
    assert(parent), assert(child);

    if (chordFlagIsActive(child->flags, FLAG_NO_AFTER)) return;

    const Property* parentAfter = propGetConst(parent, KC_PROP_AFTER);
    if (!propertyHasContent(parentAfter)) return;

    copyPropertyIfChildEmpty(parent, child, KC_PROP_AFTER);

    if (chordFlagIsActive(parent->flags, FLAG_SYNC_AFTER)) child->flags |= FLAG_SYNC_AFTER;
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
    if (!chordFlagIsActive(child, FLAG_HOLD) && chordFlagIsActive(parent, FLAG_RELEASE))
    {
        child |= FLAG_RELEASE;
    }
    if (!chordFlagIsActive(child, FLAG_RELEASE) && chordFlagIsActive(parent, FLAG_HOLD))
    {
        child |= FLAG_HOLD;
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
deduplicateVector(Vector* chords, void (*freeChord)(KeyChord*))
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
        freeChord(dup);
        vectorRemove(chords, index);

        while (!stackIsEmpty(&stack) && index == *(STACK_PEEK(&stack, size_t)))
        {
            stackPop(&stack);
        }
    }

    stackFree(&stack);
}

void
deduplicateKeyChordVector(Vector* chords)
{
    deduplicateVector(chords, freeKeyChordVectorProps);
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
