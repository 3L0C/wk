#ifndef WK_LIB_UTIL_H_
#define WK_LIB_UTIL_H_

#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "types.h"

bool vrprintf(char** ioBuffer, size_t* ioLen, const char* fmt, va_list args);
uint32_t countChords(const Chord* chords);
void calculateGrid(const uint32_t count, const uint32_t maxCols, uint32_t* rows, uint32_t* cols);

#endif /* WK_LIB_UTIL_H_ */
