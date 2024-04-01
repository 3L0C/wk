#ifndef WK_LIB_DEBUG_H_
#define WK_LIB_DEBUG_H_

#include "cairo.h"
#include "client.h"
#include "properties.h"
#include "types.h"
#include "window.h"

void debugCairoPaint(const CairoPaint* paint);
void debugChord(const Chord* chord, unsigned int indent);
void debugChords(const Chord* chords, unsigned int indent);
void debugChordsShallow(const Chord* chords, uint32_t len);
void debugClient(const Client* client);
void debugGrid(uint32_t x, uint32_t y, uint32_t r, uint32_t c, uint32_t wp, uint32_t hp, uint32_t cw, uint32_t ch, uint32_t count);
void debugHexColors(const WkHexColor* colors);
void debugKey(const Key* key);
void debugMsg(bool debug, const char* fmt, ...);
void debugProperties(const WkProperties* props);
void debugStatus(WkStatus status);

#endif /* WK_LIB_DEBUG_H_ */
