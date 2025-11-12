#ifndef WK_COMMON_DEBUG_H_
#define WK_COMMON_DEBUG_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* local includes */
#include "array.h"
#include "key_chord.h"
#include "menu.h"

void debugMsg(bool debug, const char* fmt, ...);
void debugMsgWithIndent(int indent, const char* fmt, ...);
void debugPrintHeader(const char* header);
void debugPrintHeaderWithIndent(int indent, const char* header);
void debugTextWithLineNumber(const char* text);
void debugTextLenWithLineNumber(const char* text, size_t len);
void disassembleArrayAsText(const Array* arr, const char* title);
void disassembleChordFlag(ChordFlag flag, int indent);
void disassembleGrid(uint32_t x, uint32_t y, uint32_t r, uint32_t c, uint32_t wp, uint32_t hp, uint32_t cw, uint32_t ch, uint32_t count);
void disassembleHexColors(const MenuHexColor* colors);
void disassembleKey(const Key* key);
void disassembleKeyWithoutHeader(const Key* key, int indent);
void disassembleKeyChord(const KeyChord* keyChord, int indent);
void disassembleKeyChordArray(const Array* keyChords, int indent);
void disassembleKeyChordArrayShallow(const Array* keyChords);
void disassembleKeyChordWithHeader(const KeyChord* keyChord, int indent);
void disassembleMenu(const Menu* menu);
void disassembleStatus(MenuStatus status);
void disassembleString(const String* string, const char* title, int indent);

#endif /* WK_COMMON_DEBUG_H_ */
