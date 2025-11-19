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
static uint32_t delay = 1000;
/* Max number of columns to use. */
static const uint32_t maxCols = 5;
/* Menu width. Set to '-1' for 1/2 the width of your screen. */
static const int32_t menuWidth = -1;
/* Menu gap between top/bottom of screen. Set to '-1' for a gap of 1/10th of the screen height. */
static const int32_t menuGap = -1;
/* X-Padding around key/description text in cells. */
static const uint32_t widthPadding = 6;
/* Y-Padding around key/description text in cells. */
static const uint32_t heightPadding = 2;
/* Additional padding between the outermost cells and the border. -1 = same as cell padding, 0 = no additional padding. */
static const int32_t tablePadding = -1;
/* Position to place the menu. '0' = bottom; '1' = top. */
static const uint32_t menuPosition = 0;
/* Menu border width */
static const uint32_t borderWidth = 4;
/* Menu border radius. 0 means no curve */
static const double borderRadius = 0;
/* Menu foreground color */
static const char* foreground[FOREGROUND_COLOR_LAST] = {
    "#DCD7BA", /* Key color */
    "#525259", /* Delimiter color */
    "#AF9FC9", /* Prefix color */
    "#DCD7BA", /* Chord color */
};
/* Menu background color */
static const char* background = "#181616";
/* Menu border color */
static const char* border = "#7FB4CA";
/* Default shell to run chord commands with. */
static const char* shell = "/bin/sh";
/* Pango font description i.e. 'Noto Mono, M+ 1c, ..., 16'. */
static const char* font = "monospace, 14";
/* Keys to use for chord arrays */
static const char* implicitArrayKeys = "asdfghjkl;";
/* Command wrapper prefix. Set to NULL or "" to disable. Examples: "uwsm app --", "firefox", etc. */
static const char* wrapCmd = "wrapper";

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

#endif /* WK_CONFIG_CONFIG_H_ */
