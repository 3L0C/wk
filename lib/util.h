#ifndef WK_LIB_UTIL_H_
#define WK_LIB_UTIL_H_

#include <stdint.h>

#include "menu.h"
#include "types.h"

void calculateGrid(const uint32_t count, const uint32_t maxCols, uint32_t* rows, uint32_t* cols);
void countChords(WkMenu* menu);
WkStatus handleKeypress(WkMenu* menu, Key* key);
bool isUtf8ContByte(char byte);
bool isUtf8MultiByteStartByte(char byte);
bool isUtf8StartByte(char byte);
WkStatus spawn(WkMenu* menu, const char* cmd, bool async);
bool statusIsError(WkStatus status);
bool statusIsRunning(WkStatus status);

#endif /* WK_LIB_UTIL_H_ */

