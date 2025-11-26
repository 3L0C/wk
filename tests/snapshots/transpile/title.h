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
#define ARRAY_EMPTY(T)           \
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
#define STRING_EMPTY (String){         \
    .parts  = ARRAY_EMPTY(StringPart), \
    .length = 0                        \
}
#define PROPERTY_STRING(_offset, _len)                     \
    (Property)                                             \
    {                                                      \
        .type  = PROP_TYPE_STRING,                         \
        .value = {.as_string = STRING((_offset), (_len)) } \
    }
#define PROPERTY_STRING_EMPTY                 \
    (Property)                                \
    {                                         \
        .type  = PROP_TYPE_STRING,            \
        .value = {.as_string = STRING_EMPTY } \
    }
#define PROPERTIES(...) __VA_ARGS__
#define PROPERTIES_EMPTY
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

static const char BUILTIN_SOURCE[] = "1override testecho \"override\"Second2title with keepecho \"keep\"Keep Title3title with deflagecho \"deflag\"Deflag Title4parent for inheritParent Title5child with inheritecho \"inherit\";implicitecho \"implicit\"Implicit Titleaimplicitecho \"implicit\"Implicit Titlebcommand becho \"command b executed\"cprefix with titlePrefix Titlednested commandecho \"nested command executed\"enested with own titleecho \"nested with title\"Nested Titledimplicitecho \"implicit\"Implicit Titlefimplicitecho \"implicit\"Implicit Titlegimplicitecho \"implicit\"Implicit Titlehimplicitecho \"implicit\"Implicit Titleiinterpolation testecho \"interpolation\"Key: ijimplicitecho \"implicit\"Implicit Titlekimplicitecho \"implicit\"Implicit Titlelimplicitecho \"implicit\"Implicit Titleqarray itemecho \"array\"Array Titlerarray itemecho \"array\"Array Titlesimplicitecho \"implicit\"Implicit Titlexvar testecho \"var\"My Custom Textyunicode testecho \"unicode\"Title with emojizspecial charsecho \"special\"Title with quotes and backslash";

static Array builtinKeyChords =
    ARRAY(
        KeyChord,
        22,
        KEY_CHORD(
            KEY(0, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(1, 13),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(14, 15),
                [KC_PROP_TITLE]       = PROPERTY_STRING(29, 6)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(35, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(36, 15),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(51, 11),
                [KC_PROP_TITLE]       = PROPERTY_STRING(62, 10)),
            FLAG_KEEP | FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(72, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(73, 17),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(90, 13),
                [KC_PROP_TITLE]       = PROPERTY_STRING(103, 12)),
            FLAG_DEFLAG | FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(115, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(116, 18),
                [KC_PROP_TITLE]       = PROPERTY_STRING(134, 12)),
            FLAG_NONE,
            ARRAY(
                KeyChord,
                1,
                KEY_CHORD(
                    KEY(146, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    PROPERTIES(
                        [KC_PROP_DESCRIPTION] = PROPERTY_STRING(147, 18),
                        [KC_PROP_COMMAND]     = PROPERTY_STRING(165, 14)),
                    FLAG_INHERIT | FLAG_WRITE,
                    ARRAY_EMPTY(KeyChord)))),
        KEY_CHORD(
            KEY(179, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(180, 8),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(188, 15),
                [KC_PROP_TITLE]       = PROPERTY_STRING(203, 14)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(217, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(218, 8),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(226, 15),
                [KC_PROP_TITLE]       = PROPERTY_STRING(241, 14)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(255, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(256, 9),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(265, 25)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(290, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(291, 17),
                [KC_PROP_TITLE]       = PROPERTY_STRING(308, 12)),
            FLAG_NONE,
            ARRAY(
                KeyChord,
                2,
                KEY_CHORD(
                    KEY(320, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    PROPERTIES(
                        [KC_PROP_DESCRIPTION] = PROPERTY_STRING(321, 14),
                        [KC_PROP_COMMAND]     = PROPERTY_STRING(335, 30)),
                    FLAG_WRITE,
                    ARRAY_EMPTY(KeyChord)),
                KEY_CHORD(
                    KEY(365, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    PROPERTIES(
                        [KC_PROP_DESCRIPTION] = PROPERTY_STRING(366, 21),
                        [KC_PROP_COMMAND]     = PROPERTY_STRING(387, 24),
                        [KC_PROP_TITLE]       = PROPERTY_STRING(411, 12)),
                    FLAG_WRITE,
                    ARRAY_EMPTY(KeyChord)))),
        KEY_CHORD(
            KEY(423, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(424, 8),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(432, 15),
                [KC_PROP_TITLE]       = PROPERTY_STRING(447, 14)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(461, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(462, 8),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(470, 15),
                [KC_PROP_TITLE]       = PROPERTY_STRING(485, 14)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(499, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(500, 8),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(508, 15),
                [KC_PROP_TITLE]       = PROPERTY_STRING(523, 14)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(537, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(538, 8),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(546, 15),
                [KC_PROP_TITLE]       = PROPERTY_STRING(561, 14)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(575, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(576, 18),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(594, 20),
                [KC_PROP_TITLE]       = PROPERTY_STRING(614, 6)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(620, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(621, 8),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(629, 15),
                [KC_PROP_TITLE]       = PROPERTY_STRING(644, 14)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(658, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(659, 8),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(667, 15),
                [KC_PROP_TITLE]       = PROPERTY_STRING(682, 14)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(696, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(697, 8),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(705, 15),
                [KC_PROP_TITLE]       = PROPERTY_STRING(720, 14)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(734, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(735, 10),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(745, 12),
                [KC_PROP_TITLE]       = PROPERTY_STRING(757, 11)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(768, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(769, 10),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(779, 12),
                [KC_PROP_TITLE]       = PROPERTY_STRING(791, 11)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(802, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(803, 8),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(811, 15),
                [KC_PROP_TITLE]       = PROPERTY_STRING(826, 14)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(840, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(841, 8),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(849, 10),
                [KC_PROP_TITLE]       = PROPERTY_STRING(859, 14)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(873, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(874, 12),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(886, 14),
                [KC_PROP_TITLE]       = PROPERTY_STRING(900, 16)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(916, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(917, 13),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(930, 14),
                [KC_PROP_TITLE]       = PROPERTY_STRING(944, 31)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)));

#endif /* WK_CONFIG_CONFIG_H_ */
