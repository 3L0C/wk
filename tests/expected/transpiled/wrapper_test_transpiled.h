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

static const char BUILTIN_SOURCE[] = "aAuto wrapcommandbExplicit wrapcmdcCustom wrapcmdcustomdUnwrappedcmdpPrefix unwrappedeChild inherits unwrapcmdfNo wrappercmdgMultialthChild with altcmd1altiAnother childcmd2alt";

static Array builtinKeyChords =
    ARRAY(
        KeyChord,
        7,
        KEY_CHORD(
            KEY(0, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(1, 9),
            STRING(10, 7),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(17, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(18, 13),
            STRING(31, 3),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(34, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(35, 11),
            STRING(46, 3),
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(49, 6),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(55, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(56, 9),
            STRING(65, 3),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_WRITE | FLAG_UNWRAP,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(68, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(69, 16),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_UNWRAP,
            ARRAY(
                KeyChord,
                1,
                KEY_CHORD(
                    KEY(85, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    STRING(86, 21),
                    STRING(107, 3),
                    EMPTY_STRING,
                    EMPTY_STRING,
                    EMPTY_STRING,
                    FLAG_WRITE,
                    EMPTY_ARRAY(KeyChord)))),
        KEY_CHORD(
            KEY(110, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(111, 10),
            STRING(121, 3),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_WRITE | FLAG_UNWRAP,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(124, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(125, 5),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(130, 3),
            FLAG_NONE,
            ARRAY(
                KeyChord,
                2,
                KEY_CHORD(
                    KEY(133, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    STRING(134, 14),
                    STRING(148, 4),
                    EMPTY_STRING,
                    EMPTY_STRING,
                    STRING(152, 3),
                    FLAG_WRITE,
                    EMPTY_ARRAY(KeyChord)),
                KEY_CHORD(
                    KEY(155, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    STRING(156, 13),
                    STRING(169, 4),
                    EMPTY_STRING,
                    EMPTY_STRING,
                    STRING(173, 3),
                    FLAG_WRITE,
                    EMPTY_ARRAY(KeyChord)))));

#endif /* WK_CONFIG_KEY_CHORDS_H_ */
