#ifndef WK_COMMON_DEBUG_H_
#define WK_COMMON_DEBUG_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* local includes */
#include "key_chord.h"
#include "menu.h"
#include "span.h"
#include "vector.h"

void debugMsg(bool debug, const char* fmt, ...);
void debugMsgWithIndent(int indent, const char* fmt, ...);
void debugPrintHeader(const char* header);
void debugPrintHeaderWithIndent(int indent, const char* header);
void debugTextLenWithLineNumber(const char* text, size_t len);
void debugTextWithLineNumber(const char* text);
void disassembleChordFlag(ChordFlag flag, int indent);
void disassembleGrid(uint32_t x, uint32_t y, uint32_t r, uint32_t c, uint32_t wp, uint32_t hp, uint32_t cw, uint32_t ch, uint32_t count);
void disassembleHexColors(const MenuHexColor* colors);
void disassembleKey(const Key* key);
void disassembleKeyChord(const KeyChord* keyChord, int indent);
void disassembleKeyChordSpan(const Span* keyChords, int indent);
void disassembleKeyChordSpanShallow(const Span* keyChords);
void disassembleKeyChordVector(const Vector* keyChords, int indent);
void disassembleKeyChordVectorShallow(const Vector* keyChords);
void disassembleKeyChordWithHeader(const KeyChord* keyChord, int indent);
void disassembleKeyWithoutHeader(const Key* key, int indent);
void disassembleMenu(const Menu* menu);
void disassembleStatus(MenuStatus status);
void disassembleVectorAsText(const Vector* vec, const char* title);

#endif /* WK_COMMON_DEBUG_H_ */
