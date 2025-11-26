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

static const char BUILTIN_SOURCE[] = "aA chordHello, world!pA prefixbA chordHello from inside prefix 'p b'cAnother prefixdDoneYou've reached the end!";

static Array builtinKeyChords =
    ARRAY(
        KeyChord,
        2,
        KEY_CHORD(
            KEY(0, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(1, 7),
                [KC_PROP_COMMAND]     = PROPERTY_STRING(8, 13)),
            FLAG_WRITE,
            ARRAY_EMPTY(KeyChord)),
        KEY_CHORD(
            KEY(21, 1, MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(22, 8)),
            FLAG_WRITE,
            ARRAY(
                KeyChord,
                2,
                KEY_CHORD(
                    KEY(30, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    PROPERTIES(
                        [KC_PROP_DESCRIPTION] = PROPERTY_STRING(31, 7),
                        [KC_PROP_COMMAND]     = PROPERTY_STRING(38, 30)),
                    FLAG_WRITE,
                    ARRAY_EMPTY(KeyChord)),
                KEY_CHORD(
                    KEY(68, 1, MOD_NONE, SPECIAL_KEY_NONE),
                    PROPERTIES(
                        [KC_PROP_DESCRIPTION] = PROPERTY_STRING(69, 14)),
                    FLAG_WRITE,
                    ARRAY(
                        KeyChord,
                        1,
                        KEY_CHORD(
                            KEY(83, 1, MOD_NONE, SPECIAL_KEY_NONE),
                            PROPERTIES(
                                [KC_PROP_DESCRIPTION] = PROPERTY_STRING(84, 4),
                                [KC_PROP_COMMAND]     = PROPERTY_STRING(88, 23)),
                            FLAG_WRITE,
                            ARRAY_EMPTY(KeyChord)))))));

#endif /* WK_CONFIG_CONFIG_H_ */
