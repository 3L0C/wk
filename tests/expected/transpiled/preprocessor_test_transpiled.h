#ifndef WK_CONFIG_CONFIG_H_
#define WK_CONFIG_CONFIG_H_

#include <stddef.h>
#include <stdint.h>

/* common includes */
#include "src/common/array.h"
#include "src/common/key_chord.h"
#include "src/common/menu.h"
#include "src/common/string.h"

/* Delimiter when displaying chords. */
static const char* delimiter = "\" > \"";
/* Delay between last keypress and first time displaying the menu. Value in milliseconds. */
static uint32_t delay = 0;
/* Max number of columns to use. */
static const uint32_t maxCols = 3;
/* Menu width. Set to '-1' for 1/2 the width of your screen. */
static const int32_t menuWidth = 500;
/* Menu gap between top/bottom of screen. Set to '-1' for a gap of 1/10th of the screen height. */
static const int32_t menuGap = -1;
/* X-Padding around key/description text in cells. */
static const uint32_t widthPadding = 10;
/* Y-Padding around key/description text in cells. */
static const uint32_t heightPadding = 4;
/* Additional padding between the outermost cells and the border. -1 = same as cell padding, 0 = no additional padding. */
static const int32_t tablePadding = -1;
/* Position to place the menu. '0' = bottom; '1' = top. */
static const uint32_t menuPosition = 1;
/* Menu border width */
static const uint32_t borderWidth = 2;
/* Menu border radius. 0 means no curve */
static const double borderRadius = 15;
/* Menu foreground color */
static const char* foreground[FOREGROUND_COLOR_LAST] = {
    "#ffffff", /* Key color */
    "#ffffff", /* Delimiter color */
    "#ffff00", /* Prefix color */
    "#ffffff", /* Chord color */
};
/* Menu background color */
static const char* background = "#123456";
/* Menu border color */
static const char* border = "#654321";
/* Default shell to run chord commands with. */
static const char* shell = "/usr/bin/zsh";
/* Pango font description i.e. 'Noto Mono, M+ 1c, ..., 16'. */
static const char* font = "Sans, 14";
/* Keys to use for chord arrays */
static const char* implicitArrayKeys = "asdfghjkl;";
/* Command wrapper prefix. Set to NULL or "" to disable. Examples: "uwsm app --", "firefox", etc. */
static const char* wrapCmd = NULL;

/* Builtin key chords */
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

#endif /* WK_CONFIG_CONFIG_H_ */
