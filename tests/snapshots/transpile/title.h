#ifndef WK_CONFIG_CONFIG_H_
#define WK_CONFIG_CONFIG_H_

#include <stddef.h>
#include <stdint.h>

/* common includes */
#include "src/common/key_chord.h"
#include "src/common/menu.h"
#include "src/common/span.h"
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
    [FOREGROUND_COLOR_GOTO]      = "#E6C384",
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
#define SPAN_STATIC(T, _count, ...)    \
    (Span)                             \
    {                                  \
        .data  = (T[]){ __VA_ARGS__ }, \
        .count = (_count)              \
    }
#define STRING(_str)               \
    (String){                      \
        .data   = (_str),          \
        .length = sizeof(_str) - 1 \
    }
#define STRING_EMPTY (String){ .data = "", .length = 0 }
#define PROPERTY_STRING(_str)                 \
    (Property)                                \
    {                                         \
        .type  = PROP_TYPE_STRING,            \
        .value = {.as_string = STRING(_str) } \
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
#define KEY(_repr, _mods, _special) \
    (Key)                           \
    {                               \
        .repr    = STRING(_repr),   \
        .mods    = (_mods),         \
        .special = (_special)       \
    }

static Span builtinKeyChords =
    SPAN_STATIC(
        KeyChord,
        24,
        KEY_CHORD(
            KEY("1", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("override test"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"override\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Second")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("2", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("title with keep"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"keep\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Keep Title")),
            FLAG_KEEP | FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("3", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("title with deflag"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"deflag\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Deflag Title")),
            FLAG_DEFLAG | FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("4", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("parent for inherit"),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Parent Title")),
            FLAG_NONE,
            SPAN_STATIC(
                KeyChord,
                1,
                KEY_CHORD(
                    KEY("5", MOD_NONE, SPECIAL_KEY_NONE),
                    PROPERTIES(
                        [KC_PROP_DESCRIPTION] = PROPERTY_STRING("child with inherit"),
                        [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"inherit\"")),
                    FLAG_INHERIT | FLAG_WRITE,
                    SPAN_EMPTY))),
        KEY_CHORD(
            KEY("6", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("title from description"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"title from desc\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("title from description")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("7", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("prefix title from desc"),
                [KC_PROP_TITLE]       = PROPERTY_STRING("prefix title from desc")),
            FLAG_NONE,
            SPAN_STATIC(
                KeyChord,
                1,
                KEY_CHORD(
                    KEY("8", MOD_NONE, SPECIAL_KEY_NONE),
                    PROPERTIES(
                        [KC_PROP_DESCRIPTION] = PROPERTY_STRING("child of prefix"),
                        [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"child\"")),
                    FLAG_WRITE,
                    SPAN_EMPTY))),
        KEY_CHORD(
            KEY(";", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("implicit"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"implicit\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Implicit Title")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("a", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("implicit"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"implicit\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Implicit Title")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("b", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("command b"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"command b executed\"")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("c", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("prefix with title"),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Prefix Title")),
            FLAG_NONE,
            SPAN_STATIC(
                KeyChord,
                2,
                KEY_CHORD(
                    KEY("d", MOD_NONE, SPECIAL_KEY_NONE),
                    PROPERTIES(
                        [KC_PROP_DESCRIPTION] = PROPERTY_STRING("nested command"),
                        [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"nested command executed\"")),
                    FLAG_WRITE,
                    SPAN_EMPTY),
                KEY_CHORD(
                    KEY("e", MOD_NONE, SPECIAL_KEY_NONE),
                    PROPERTIES(
                        [KC_PROP_DESCRIPTION] = PROPERTY_STRING("nested with own title"),
                        [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"nested with title\""),
                        [KC_PROP_TITLE]       = PROPERTY_STRING("Nested Title")),
                    FLAG_WRITE,
                    SPAN_EMPTY))),
        KEY_CHORD(
            KEY("d", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("implicit"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"implicit\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Implicit Title")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("f", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("implicit"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"implicit\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Implicit Title")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("g", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("implicit"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"implicit\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Implicit Title")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("h", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("implicit"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"implicit\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Implicit Title")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("i", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("interpolation test"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"interpolation\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Key: i")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("j", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("implicit"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"implicit\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Implicit Title")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("k", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("implicit"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"implicit\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Implicit Title")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("l", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("implicit"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"implicit\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Implicit Title")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("q", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("array item"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"array\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Array Title")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("r", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("array item"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"array\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Array Title")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("s", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("implicit"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"implicit\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Implicit Title")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("x", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("var test"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"var\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("My Custom Text")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("y", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("unicode test"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"unicode\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Title with emoji")),
            FLAG_WRITE,
            SPAN_EMPTY),
        KEY_CHORD(
            KEY("z", MOD_NONE, SPECIAL_KEY_NONE),
            PROPERTIES(
                [KC_PROP_DESCRIPTION] = PROPERTY_STRING("special chars"),
                [KC_PROP_COMMAND]     = PROPERTY_STRING("echo \"special\""),
                [KC_PROP_TITLE]       = PROPERTY_STRING("Title with quotes and backslash")),
            FLAG_WRITE,
            SPAN_EMPTY));

#endif /* WK_CONFIG_CONFIG_H_ */
