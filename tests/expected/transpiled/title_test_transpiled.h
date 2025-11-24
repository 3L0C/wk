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
#define PROP_STRING(_offset, _len)                              \
    (Property)                                                  \
    {                                                           \
        .type  = PROP_TYPE_STRING,                              \
        .value = {.as_description = STRING((_offset), (_len)) } \
    }
#define EMPTY_PROP_STRING                          \
    (Property)                                     \
    {                                              \
        .type  = PROP_TYPE_STRING,                 \
        .value = {.as_description = EMPTY_STRING } \
    }
#define PROPS(...) __VA_ARGS__
#define EMPTY_PROPS
#define KEY_CHORD(_key, _props, _flags, _chords) \
    (KeyChord)                                   \
    {                                            \
        .key       = (_key),                     \
        .props     = { _props },                 \
        .flags     = (_flags),                   \
        .keyChords = (_chords)                   \
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
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(1, 8),
                [PROP_COMMAND]     = PROP_STRING(9, 15),
                [PROP_TITLE]       = PROP_STRING(24, 14)),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(38, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(39, 9),
                [PROP_COMMAND]     = PROP_STRING(48, 25)),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(73, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(74, 17),
                [PROP_TITLE]       = PROP_STRING(91, 12)),
            FLAG_NONE,
            ARRAY(
                KeyChord,
                2,
                KEY_CHORD(
                    KEY(103, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    PROPS(
                        [PROP_DESCRIPTION] = PROP_STRING(104, 14),
                        [PROP_COMMAND]     = PROP_STRING(118, 30)),
                    FLAG_WRITE,
                    EMPTY_ARRAY(KeyChord)),
                KEY_CHORD(
                    KEY(148, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    PROPS(
                        [PROP_DESCRIPTION] = PROP_STRING(149, 21),
                        [PROP_COMMAND]     = PROP_STRING(170, 24),
                        [PROP_TITLE]       = PROP_STRING(194, 12)),
                    FLAG_WRITE,
                    EMPTY_ARRAY(KeyChord)))),
        KEY_CHORD(
            KEY(206, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(207, 8),
                [PROP_COMMAND]     = PROP_STRING(215, 15),
                [PROP_TITLE]       = PROP_STRING(230, 14)),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(244, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(245, 8),
                [PROP_COMMAND]     = PROP_STRING(253, 15),
                [PROP_TITLE]       = PROP_STRING(268, 14)),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(282, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(283, 18),
                [PROP_COMMAND]     = PROP_STRING(301, 20),
                [PROP_TITLE]       = PROP_STRING(321, 6)),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(327, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(328, 8),
                [PROP_COMMAND]     = PROP_STRING(336, 15),
                [PROP_TITLE]       = PROP_STRING(351, 14)),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(365, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(366, 10),
                [PROP_COMMAND]     = PROP_STRING(376, 12),
                [PROP_TITLE]       = PROP_STRING(388, 11)),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(399, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(400, 10),
                [PROP_COMMAND]     = PROP_STRING(410, 12),
                [PROP_TITLE]       = PROP_STRING(422, 11)),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(433, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(434, 8),
                [PROP_COMMAND]     = PROP_STRING(442, 15),
                [PROP_TITLE]       = PROP_STRING(457, 14)),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(471, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(472, 8),
                [PROP_COMMAND]     = PROP_STRING(480, 15),
                [PROP_TITLE]       = PROP_STRING(495, 14)),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(509, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(510, 8),
                [PROP_COMMAND]     = PROP_STRING(518, 15),
                [PROP_TITLE]       = PROP_STRING(533, 14)),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(547, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(548, 8),
                [PROP_COMMAND]     = PROP_STRING(556, 15),
                [PROP_TITLE]       = PROP_STRING(571, 14)),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(585, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(586, 8),
                [PROP_COMMAND]     = PROP_STRING(594, 15),
                [PROP_TITLE]       = PROP_STRING(609, 14)),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(623, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(624, 8),
                [PROP_COMMAND]     = PROP_STRING(632, 15),
                [PROP_TITLE]       = PROP_STRING(647, 14)),
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(661, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(662, 8),
                [PROP_COMMAND]     = PROP_STRING(670, 10),
                [PROP_TITLE]       = PROP_STRING(680, 14)),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(694, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(695, 12),
                [PROP_COMMAND]     = PROP_STRING(707, 14),
                [PROP_TITLE]       = PROP_STRING(721, 16)),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(737, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(738, 13),
                [PROP_COMMAND]     = PROP_STRING(751, 14),
                [PROP_TITLE]       = PROP_STRING(765, 31)),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(796, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(797, 13),
                [PROP_COMMAND]     = PROP_STRING(810, 15),
                [PROP_TITLE]       = PROP_STRING(825, 11)),
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(836, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(837, 15),
                [PROP_COMMAND]     = PROP_STRING(852, 11),
                [PROP_TITLE]       = PROP_STRING(863, 10)),
            FLAG_KEEP | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(873, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(874, 17),
                [PROP_COMMAND]     = PROP_STRING(891, 13),
                [PROP_TITLE]       = PROP_STRING(904, 12)),
            FLAG_DEFLAG | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(916, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPS(
                [PROP_DESCRIPTION] = PROP_STRING(917, 18),
                [PROP_TITLE]       = PROP_STRING(935, 12)),
            FLAG_NONE,
            ARRAY(
                KeyChord,
                1,
                KEY_CHORD(
                    KEY(947, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    PROPS(
                        [PROP_DESCRIPTION] = PROP_STRING(948, 18),
                        [PROP_COMMAND]     = PROP_STRING(966, 14)),
                    FLAG_INHERIT | FLAG_WRITE,
                    EMPTY_ARRAY(KeyChord)))));

#endif /* WK_CONFIG_CONFIG_H_ */
