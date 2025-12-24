#ifndef WK_COMPILER_TRANSFORM_H_
#define WK_COMPILER_TRANSFORM_H_

#include <stdbool.h>

/* common includes */
#include "common/menu.h"
#include "common/span.h"
#include "common/vector.h"

/* local includes */
#include "scanner.h"

void deduplicateKeyChordVector(Vector* chords);
void deduplicateVector(Vector* chords, void (*freeChord)(KeyChord*));
void keyChordSpanSort(Span* chords);
void propagateInheritanceSpan(Span* chords);
bool transform(Vector* chords, Menu* menu, Scanner* scanner);

#endif /* WK_COMPILER_TRANSFORM_H_ */
