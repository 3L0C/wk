#ifndef WK_LIB_DEBUG_H_
#define WK_LIB_DEBUG_H_

#include "cairo.h"
#include "client.h"
#include "properties.h"
#include "types.h"

void debugCairoPaint(const CairoPaint* paint);
void debugChord(const Chord* chord, unsigned int indent);
void debugChords(const Chord* chords, unsigned int indent);
void debugChordsShallow(const Chord* chords, uint32_t len);
void debugClient(const Client* client);
void debugHexColors(const WkHexColor* colors);
void debugKey(const Key* key);
void debugMsg(bool debug, const char* fmt, ...);
void debugProperties(const WkProperties* props);

#endif /* WK_LIB_DEBUG_H_ */
