#ifndef WK_LIB_UTIL_H_
#define WK_LIB_UTIL_H_

#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "properties.h"
#include "types.h"
#include "window.h"

void calculateGrid(const uint32_t count, const uint32_t maxCols, uint32_t* rows, uint32_t* cols);
void countChords(WkProperties* props);
WkStatus handleKeypress(WkProperties* props, Key* key);
bool isUtf8StartByte(char byte);
WkStatus spawn(const char* shell, const char* cmd);
WkStatus spawnAsync(WkProperties* props, const char* cmd, bool waitFlag);

#endif /* WK_LIB_UTIL_H_ */

