#ifndef WK_COMMON_UTIL_H_
#define WK_COMMON_UTIL_H_

#include <stdint.h>

/* local includes */
#include "menu.h"
#include "key_chord.h"

void calculateGrid(const uint32_t count, const uint32_t maxCols, uint32_t* rows, uint32_t* cols);
uint32_t countKeyChords(const KeyChord* keyChords);
MenuStatus handleKeypress(Menu* menu, const Key* key);
bool isUtf8ContByte(char byte);
bool isUtf8MultiByteStartByte(char byte);
bool isUtf8StartByte(char byte);
MenuStatus spawn(const Menu* menu, const char* cmd, bool async);
bool statusIsError(MenuStatus status);
bool statusIsRunning(MenuStatus status);

#endif /* WK_COMMON_UTIL_H_ */
