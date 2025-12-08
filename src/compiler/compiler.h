#ifndef WK_COMPILER_COMPILER_H_
#define WK_COMPILER_COMPILER_H_

/* common includes */
#include "common/key_chord.h"
#include "common/menu.h"
#include "common/span.h"
#include "common/vector.h"

Span* compile(Menu* menu, const char* filepath);
void  compilerInitChord(KeyChord* chord);
void  compilerFreeChord(KeyChord* chord);
void  compilerFreeChordSpan(Span* span);
void  compilerFreeChordVector(Vector* vec);

#endif /* WK_COMPILER_COMPILER_H_ */
