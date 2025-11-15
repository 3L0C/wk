#ifndef WK_CONFIG_KEY_CHORDS_H_
#define WK_CONFIG_KEY_CHORDS_H_

#include <stddef.h>

/* common includes */
#include "src/common/array.h"
#include "src/common/key_chord.h"
#include "src/common/string.h"

#define EMPTY_ARRAY(T)           \
    (Array)                      \
    {                            \
        .data        = NULL,     \
        .length      = 0,        \
        .capacity    = 0,        \
        .elementSize = sizeof(T) \
    }

static const char BUILTIN_SOURCE[] = "";

static Array builtinKeyChords = EMPTY_ARRAY(KeyChord);

#endif /* WK_CONFIG_KEY_CHORDS_H_ */
