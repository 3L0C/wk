#ifndef WK_CONFIG_KEY_CHORDS_H_
#define WK_CONFIG_KEY_CHORDS_H_

#include <stddef.h>

/* common includes */
#include "src/common/array.h"
#include "src/common/key_chord.h"
#include "src/common/string.h"

#define ARRAY(T, _len, ...)                  \
    (Array)                                  \
    {                                        \
        .data        = (T[]){ __VA_ARGS__ }, \
        .length      = (_len),               \
        .capacity    = (_len),               \
        .elementSize = sizeof(T)             \
    }
#define EMPTY_ARRAY(T)           \
    (Array)                      \
    {                            \
        .data        = NULL,     \
        .length      = 0,        \
        .capacity    = 0,        \
        .elementSize = sizeof(T) \
    }
#define STRING(_offset, _len)                                                                       \
    (String)                                                                                        \
    {                                                                                               \
        .parts  = ARRAY(StringPart, 1, { .source = BUILTIN_SOURCE + (_offset), .length = (_len) }), \
        .length = (_len)                                                                            \
    }
#define EMPTY_STRING (String){         \
    .parts  = EMPTY_ARRAY(StringPart), \
    .length = 0                        \
}
#define KEY_CHORD(_key, _desc, _cmd, _before, _after, _wrap_cmd, _flags, _chords) \
    (KeyChord)                                                                    \
    {                                                                             \
        .key         = (_key),                                                    \
        .description = (_desc),                                                   \
        .command     = (_cmd),                                                    \
        .before      = (_before),                                                 \
        .after       = (_after),                                                  \
        .wrapCmd     = (_wrap_cmd),                                               \
        .flags       = (_flags),                                                  \
        .keyChords   = (_chords)                                                  \
    }
#define KEY(_offset, _len, _mods, _special)   \
    (Key)                                     \
    {                                         \
        .repr    = STRING((_offset), (_len)), \
        .mods    = (_mods),                   \
        .special = (_special)                 \
    }

static const char BUILTIN_SOURCE[] = "aA chordHello, world!bBasicb - BasicpA prefixbA chordHello from inside prefix 'p b'cAnother prefixdDoneYou've reached the end!";

static Array builtinKeyChords =
    ARRAY(
        KeyChord,
        3,
        KEY_CHORD(
            KEY(0, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(1, 7),
            STRING(8, 13),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(21, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(22, 5),
            STRING(27, 9),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(36, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(37, 8),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_WRITE,
            ARRAY(
                KeyChord,
                2,
                KEY_CHORD(
                    KEY(45, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    STRING(46, 7),
                    STRING(53, 30),
                    EMPTY_STRING,
                    EMPTY_STRING,
                    EMPTY_STRING,
                    FLAG_WRITE,
                    EMPTY_ARRAY(KeyChord)),
                KEY_CHORD(
                    KEY(83, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    STRING(84, 14),
                    EMPTY_STRING,
                    EMPTY_STRING,
                    EMPTY_STRING,
                    EMPTY_STRING,
                    FLAG_WRITE,
                    ARRAY(
                        KeyChord,
                        1,
                        KEY_CHORD(
                            KEY(98, 1, MOD_NONE, SPECIAL_KEY_NONE),
                            STRING(99, 4),
                            STRING(103, 23),
                            EMPTY_STRING,
                            EMPTY_STRING,
                            EMPTY_STRING,
                            FLAG_WRITE,
                            EMPTY_ARRAY(KeyChord)))))));

#endif /* WK_CONFIG_KEY_CHORDS_H_ */
