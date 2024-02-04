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
bool isUtf8ContByte(char byte);
bool isUtf8MultiByteStartByte(char byte);
bool isUtf8StartByte(char byte);
WkStatus spawn(WkProperties* props, const char* cmd, bool async);

#endif /* WK_LIB_UTIL_H_ */

