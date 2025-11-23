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
static const char* delimiter = " -> ";
/* Delay between last keypress and first time displaying the menu. Value in milliseconds. */
static uint32_t delay = 1000;
/* Delay in milliseconds after ungrab before command execution for +keep chords. */
static uint32_t keepDelay = 75;
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
    [FOREGROUND_COLOR_KEY]       = "#DCD7BA",
    [FOREGROUND_COLOR_DELIMITER] = "#525259",
    [FOREGROUND_COLOR_PREFIX]    = "#AF9FC9",
    [FOREGROUND_COLOR_CHORD]     = "#DCD7BA",
    [FOREGROUND_COLOR_TITLE]     = "#ff0000",
};
/* Menu background color */
static const char* background = "#181616";
/* Menu border color */
static const char* border = "#7FB4CA";
/* Default shell to run chord commands with. */
static const char* shell = "/bin/sh";
/* Pango font description i.e. 'Noto Mono, M+ 1c, ..., 16'. */
static const char* font = "monospace, 14";
/* Pango font description i.e. 'Inter Nerd Font, M+ 1c, ..., 16'. */
static const char* titleFont = "sans-serif, 18";
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
#define KEY_CHORD(_key, _desc, _cmd, _before, _after, _wrap_cmd, _title, _flags, _chords) \
    (KeyChord)                                                                            \
    {                                                                                     \
        .key         = (_key),                                                            \
        .description = (_desc),                                                           \
        .command     = (_cmd),                                                            \
        .before      = (_before),                                                         \
        .after       = (_after),                                                          \
        .wrapCmd     = (_wrap_cmd),                                                       \
        .title       = (_title),                                                          \
        .flags       = (_flags),                                                          \
        .keyChords   = (_chords)                                                          \
    }
#define KEY(_offset, _len, _mods, _special)   \
    (Key)                                     \
    {                                         \
        .repr    = STRING((_offset), (_len)), \
        .mods    = (_mods),                   \
        .special = (_special)                 \
    }

static const char BUILTIN_SOURCE[] = "aimplicitecho \"implicit\"Implicit Titlebcommand becho \"command b executed\"cprefix with titlePrefix Titlednested commandecho \"nested command executed\"enested with own titleecho \"nested with title\"Nested Titlefimplicitecho \"implicit\"Implicit Titlehimplicitecho \"implicit\"Implicit Titleiinterpolation testecho \"interpolation\"Key: ikimplicitecho \"implicit\"Implicit Titleqarray itemecho \"array\"Array Titlerarray itemecho \"array\"Array Titlesimplicitecho \"implicit\"Implicit Titledimplicitecho \"implicit\"Implicit Titlegimplicitecho \"implicit\"Implicit Titlejimplicitecho \"implicit\"Implicit Titlelimplicitecho \"implicit\"Implicit Title;implicitecho \"implicit\"Implicit Titlexvar testecho \"var\"My Custom Textyunicode testecho \"unicode\"Title with emojizspecial charsecho \"special\"Title with quotes and backslash1override testecho \"override\"FirstSecond2title with keepecho \"keep\"Keep Title3title with deflagecho \"deflag\"Deflag Title4parent for inheritParent Title5child with inheritecho \"inherit\"";

static Array builtinKeyChords =
    ARRAY(
        KeyChord,
        22,
        KEY_CHORD(
            KEY(0, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(1, 8),
            STRING(9, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(24, 14),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(38, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(39, 9),
            STRING(48, 25),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(73, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(74, 17),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(91, 12),
            FLAG_NONE,
            ARRAY(
                KeyChord,
                2,
                KEY_CHORD(
                    KEY(103, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    STRING(104, 14),
                    STRING(118, 30),
                    EMPTY_STRING,
                    EMPTY_STRING,
                    EMPTY_STRING,
                    EMPTY_STRING,
                    FLAG_WRITE,
                    EMPTY_ARRAY(KeyChord)),
                KEY_CHORD(
                    KEY(148, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    STRING(149, 21),
                    STRING(170, 24),
                    EMPTY_STRING,
                    EMPTY_STRING,
                    EMPTY_STRING,
                    STRING(194, 12),
                    FLAG_WRITE,
                    EMPTY_ARRAY(KeyChord)))),
        KEY_CHORD(
            KEY(206, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(207, 8),
            STRING(215, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(230, 14),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(244, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(245, 8),
            STRING(253, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(268, 14),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(282, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(283, 18),
            STRING(301, 20),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(321, 6),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(327, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(328, 8),
            STRING(336, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(351, 14),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(365, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(366, 10),
            STRING(376, 12),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(388, 11),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(399, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(400, 10),
            STRING(410, 12),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(422, 11),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(433, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(434, 8),
            STRING(442, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(457, 14),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(471, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(472, 8),
            STRING(480, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(495, 14),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(509, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(510, 8),
            STRING(518, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(533, 14),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(547, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(548, 8),
            STRING(556, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(571, 14),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(585, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(586, 8),
            STRING(594, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(609, 14),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(623, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(624, 8),
            STRING(632, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(647, 14),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(661, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(662, 8),
            STRING(670, 10),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(680, 14),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(694, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(695, 12),
            STRING(707, 14),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(721, 16),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(737, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(738, 13),
            STRING(751, 14),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(765, 31),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(796, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(797, 13),
            STRING(810, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(825, 11),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(836, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(837, 15),
            STRING(852, 11),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(863, 10),
            FLAG_KEEP | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(873, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(874, 17),
            STRING(891, 13),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(904, 12),
            FLAG_DEFLAG | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(916, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(917, 18),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            STRING(935, 12),
            FLAG_NONE,
            ARRAY(
                KeyChord,
                1,
                KEY_CHORD(
                    KEY(947, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    STRING(948, 18),
                    STRING(966, 14),
                    EMPTY_STRING,
                    EMPTY_STRING,
                    EMPTY_STRING,
                    EMPTY_STRING,
                    FLAG_INHERIT | FLAG_WRITE,
                    EMPTY_ARRAY(KeyChord)))));

#endif /* WK_CONFIG_CONFIG_H_ */
