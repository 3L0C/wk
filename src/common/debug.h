#ifndef WK_COMMON_DEBUG_H_
#define WK_COMMON_DEBUG_H_

#include "menu.h"
#include "types.h"

void debugKeyChord(const WkKeyChord* keyChord, unsigned int indent);
void debugKeyChords(const WkKeyChord* keyChords, unsigned int indent);
void debugKeyChordsShallow(const WkKeyChord* keyChords, uint32_t len);
void debugGrid(uint32_t x, uint32_t y, uint32_t r, uint32_t c, uint32_t wp, uint32_t hp, uint32_t cw, uint32_t ch, uint32_t count);
void debugHexColors(const WkHexColor* colors);
void debugKey(const WkKey* key);
void debugMenu(const WkMenu* menu);
void debugMsg(bool debug, const char* fmt, ...);
void debugStatus(WkStatus status);
void debugStringWithIndent(const char* text);
void debugStringLenWithIndent(const char* text, size_t len);

#endif /* WK_COMMON_DEBUG_H_ */
