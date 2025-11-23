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
    [FOREGROUND_COLOR_TITLE]     = "#DCD7BA",
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
static const char* titleFont = "sans-serif, 16";
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

static const char BUILTIN_SOURCE[] = "aSwitch 1switching to 0sSwitch 2switching to 1dSwitch 3switching to 2fSwitch 4switching to 3gSwitch 5switching to 4hSwitch 6switching to 5jSwitch 7switching to 6kSwitch 8switching to 7lSwitch 9switching to 8;Switch 10switching to 9xSwitch 11switching to 10ySwitch 12switching to 11zSwitch 13switching to 12aSwitch 14switching to 13sSwitch 15switching to 14dSwitch 16switching to 15fSwitch 17switching to 16gSwitch 18switching to 17hSwitch 19switching to 18jSwitch 20switching to 19kSwitch 21switching to 20lSwitch 22switching to 21;Switch 23switching to 22";

static Array builtinKeyChords =
    ARRAY(
        KeyChord,
        23,
        KEY_CHORD(
            KEY(0, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(1, 8),
            STRING(9, 14),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(23, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(24, 8),
            STRING(32, 14),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(46, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(47, 8),
            STRING(55, 14),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(69, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(70, 8),
            STRING(78, 14),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(92, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(93, 8),
            STRING(101, 14),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(115, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(116, 8),
            STRING(124, 14),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(138, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(139, 8),
            STRING(147, 14),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(161, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(162, 8),
            STRING(170, 14),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(184, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(185, 8),
            STRING(193, 14),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(207, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(208, 9),
            STRING(217, 14),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(231, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(232, 9),
            STRING(241, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(256, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(257, 9),
            STRING(266, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(281, 1, MOD_NONE, SPECIAL_KEY_NONE),
            STRING(282, 9),
            STRING(291, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(306, 1, MOD_CTRL | MOD_META, SPECIAL_KEY_NONE),
            STRING(307, 9),
            STRING(316, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(331, 1, MOD_CTRL | MOD_META, SPECIAL_KEY_NONE),
            STRING(332, 9),
            STRING(341, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(356, 1, MOD_CTRL | MOD_META, SPECIAL_KEY_NONE),
            STRING(357, 9),
            STRING(366, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(381, 1, MOD_CTRL | MOD_META, SPECIAL_KEY_NONE),
            STRING(382, 9),
            STRING(391, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(406, 1, MOD_CTRL | MOD_META, SPECIAL_KEY_NONE),
            STRING(407, 9),
            STRING(416, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(431, 1, MOD_CTRL | MOD_META, SPECIAL_KEY_NONE),
            STRING(432, 9),
            STRING(441, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(456, 1, MOD_CTRL | MOD_META, SPECIAL_KEY_NONE),
            STRING(457, 9),
            STRING(466, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(481, 1, MOD_CTRL | MOD_META, SPECIAL_KEY_NONE),
            STRING(482, 9),
            STRING(491, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(506, 1, MOD_CTRL | MOD_META, SPECIAL_KEY_NONE),
            STRING(507, 9),
            STRING(516, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)),
        KEY_CHORD(
            KEY(531, 1, MOD_CTRL | MOD_META, SPECIAL_KEY_NONE),
            STRING(532, 9),
            STRING(541, 15),
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            EMPTY_STRING,
            FLAG_IGNORE_SORT | FLAG_WRITE,
            EMPTY_ARRAY(KeyChord)));

#endif /* WK_CONFIG_CONFIG_H_ */
