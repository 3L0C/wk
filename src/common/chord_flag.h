#ifndef WK_COMMON_CHORD_FLAG_H_
#define WK_COMMON_CHORD_FLAG_H_

#include <stdbool.h>
#include <stdint.h>

typedef uint16_t ChordFlag;
enum
{
    FLAG_NONE         = 0,
    FLAG_KEEP         = 1 << 0,
    FLAG_CLOSE        = 1 << 1,
    FLAG_INHERIT      = 1 << 2,
    FLAG_IGNORE       = 1 << 3,
    FLAG_UNHOOK       = 1 << 4,
    FLAG_DEFLAG       = 1 << 5,
    FLAG_NO_BEFORE    = 1 << 6,
    FLAG_NO_AFTER     = 1 << 7,
    FLAG_WRITE        = 1 << 8,
    FLAG_EXECUTE      = 1 << 9,
    FLAG_SYNC_COMMAND = 1 << 10,
    FLAG_SYNC_BEFORE  = 1 << 11,
    FLAG_SYNC_AFTER   = 1 << 12,
    FLAG_UNWRAP       = 1 << 13,
};

int       chordFlagCount(ChordFlag flag);
bool      chordFlagHasAnyActive(ChordFlag flag);
ChordFlag chordFlagInit(void);
bool      chordFlagIsActive(ChordFlag flag, ChordFlag test);
bool      chordFlagsAreDefault(ChordFlag flag);

#endif /* WK_COMMON_CHORD_FLAG_H_ */
