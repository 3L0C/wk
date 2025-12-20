#ifndef WK_COMPILER_TRANSFORM_H_
#define WK_COMPILER_TRANSFORM_H_

#include <stdbool.h>

/* common includes */
#include "common/menu.h"
#include "common/span.h"
#include "common/vector.h"

/* local includes */
#include "scanner.h"

bool transform(Vector* chords, Menu* menu, Scanner* scanner);
void deduplicateVector(Vector* chords, void (*freeChord)(KeyChord*));
void deduplicateKeyChordVector(Vector* chords);
void keyChordSpanSort(Span* chords);
void propagateInheritanceSpan(Span* chords);

#endif /* WK_COMPILER_TRANSFORM_H_ */
