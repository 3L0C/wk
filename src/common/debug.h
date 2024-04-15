#ifndef WK_COMMON_DEBUG_H_
#define WK_COMMON_DEBUG_H_

#include "menu.h"
#include "types.h"

void debugMsg(bool debug, const char* fmt, ...);
void debugMsgWithIndent(int indent, const char* fmt, ...);
void debugPrintHeader(const char* header);
void debugPrintHeaderWithIndent(int indent, const char* header);
void debugTextWithLineNumber(const char* text);
void debugTextLenWithLineNumber(const char* text, size_t len);
void disassembleGrid(uint32_t x, uint32_t y, uint32_t r, uint32_t c, uint32_t wp, uint32_t hp, uint32_t cw, uint32_t ch, uint32_t count);
void disassembleHexColors(const WkHexColor* colors);
void disassembleKey(const WkKey* key);
void disassembleKeyChord(const WkKeyChord* keyChord, int indent);
void disassembleKeyChords(const WkKeyChord* keyChords, int indent);
void disassembleKeyChordsShallow(const WkKeyChord* keyChords, uint32_t len);
void disassembleKeyChordWithHeader(const WkKeyChord* keyChord, int indent);
void disassembleMenu(const WkMenu* menu);
void disassembleStatus(WkStatus status);

#endif /* WK_COMMON_DEBUG_H_ */
