#ifndef WK_COMMON_DEBUG_H_
#define WK_COMMON_DEBUG_H_

#include "menu.h"
#include "key_chord.h"

void debugMsg(bool debug, const char* fmt, ...);
void debugMsgWithIndent(int indent, const char* fmt, ...);
void debugPrintHeader(const char* header);
void debugPrintHeaderWithIndent(int indent, const char* header);
void debugTextWithLineNumber(const char* text);
void debugTextLenWithLineNumber(const char* text, size_t len);
void disassembleFlags(const KeyChordFlags* flags, int indent);
void disassembleGrid(uint32_t x, uint32_t y, uint32_t r, uint32_t c, uint32_t wp, uint32_t hp, uint32_t cw, uint32_t ch, uint32_t count);
void disassembleHexColors(const MenuHexColor* colors);
void disassembleKey(const KeyChordKey* key);
void disassembleKeyChord(const KeyChord* keyChord, int indent);
void disassembleKeyChords(const KeyChord* keyChords, int indent);
void disassembleKeyChordsShallow(const KeyChord* keyChords, uint32_t len);
void disassembleKeyChordWithHeader(const KeyChord* keyChord, int indent);
void disassembleMenu(const Menu* menu);
void disassembleStatus(MenuStatus status);

#endif /* WK_COMMON_DEBUG_H_ */
