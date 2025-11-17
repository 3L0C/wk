#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

/* common includes */
#include "common/array.h"
#include "common/common.h"
#include "common/key_chord.h"
#include "common/menu.h"
#include "common/string.h"

static size_t offset = 0;

static void writeKeyChords(const Array* keyChords, int indent);

static void
writeNewlineWithIndent(int indent)
{
    printf("\n%*s", indent * 4, " ");
}

static void
writeConfigHeader(void)
{
    printf(
        "#ifndef WK_CONFIG_CONFIG_H_\n"
        "#define WK_CONFIG_CONFIG_H_\n"
        "\n"
        "#include <stddef.h>\n"
        "#include <stdint.h>\n"
        "\n"
        "/* common includes */\n"
        "#include \"src/common/array.h\"\n"
        "#include \"src/common/key_chord.h\"\n"
        "#include \"src/common/menu.h\"\n"
        "#include \"src/common/string.h\"\n"
        "\n");
}

static void
writeEscString(const String* string)
{
    if (!string) return;
    if (stringIsEmpty(string)) return;

    StringIterator iter = stringIteratorMake(string);
    char           c    = '\0';
    while ((c = stringIteratorNext(&iter)) != '\0')
    {
        switch (c)
        {
        case '\\': printf("\\\\"); break;
        case '\"': printf("\\\""); break;
        case '\n': printf("\\\n"); break;
        default: printf("%c", c); break;
        }
    }
}

static void
writeConfigVariables(const Menu* menu)
{
    assert(menu);

    /* Delimiter */
    printf("/* Delimiter when displaying chords. */\n");
    printf("static const char* delimiter = \"%s\";\n", menu->delimiter);

    /* Delay */
    printf("/* Delay between last keypress and first time displaying the menu. Value in milliseconds. */\n");
    printf("static uint32_t delay = %u;\n", menu->delay);

    /* Max columns */
    printf("/* Max number of columns to use. */\n");
    printf("static const uint32_t maxCols = %u;\n", menu->maxCols);

    /* Menu width */
    printf("/* Menu width. Set to '-1' for 1/2 the width of your screen. */\n");
    printf("static const int32_t menuWidth = %d;\n", menu->menuWidth);

    /* Menu gap */
    printf("/* Menu gap between top/bottom of screen. Set to '-1' for a gap of 1/10th of the screen height. */\n");
    printf("static const int32_t menuGap = %d;\n", menu->menuGap);

    /* Width padding */
    printf("/* X-Padding around key/description text in cells. */\n");
    printf("static const uint32_t widthPadding = %u;\n", menu->wpadding);

    /* Height padding */
    printf("/* Y-Padding around key/description text in cells. */\n");
    printf("static const uint32_t heightPadding = %u;\n", menu->hpadding);

    /* Table padding */
    printf("/* Additional padding between the outermost cells and the border. -1 = same as cell padding, 0 = no additional padding. */\n");
    printf("static const int32_t tablePadding = %d;\n", menu->tablePadding);

    /* Menu position */
    printf("/* Position to place the menu. '0' = bottom; '1' = top. */\n");
    printf("static const uint32_t menuPosition = %u;\n", (uint32_t)menu->position);

    /* Border width */
    printf("/* Menu border width */\n");
    printf("static const uint32_t borderWidth = %u;\n", menu->borderWidth);

    /* Border radius */
    printf("/* Menu border radius. 0 means no curve */\n");
    printf("static const double borderRadius = %g;\n", menu->borderRadius);

    /* Foreground colors */
    printf("/* Menu foreground color */\n");
    printf("static const char* foreground[FOREGROUND_COLOR_LAST] = {\n");
    printf("    \"%s\", /* Key color */\n", menu->colors[MENU_COLOR_KEY].hex);
    printf("    \"%s\", /* Delimiter color */\n", menu->colors[MENU_COLOR_DELIMITER].hex);
    printf("    \"%s\", /* Prefix color */\n", menu->colors[MENU_COLOR_PREFIX].hex);
    printf("    \"%s\", /* Chord color */\n", menu->colors[MENU_COLOR_CHORD].hex);
    printf("};\n");

    /* Background color */
    printf("/* Menu background color */\n");
    printf("static const char* background = \"%s\";\n", menu->colors[MENU_COLOR_BACKGROUND].hex);

    /* Border color */
    printf("/* Menu border color */\n");
    printf("static const char* border = \"%s\";\n", menu->colors[MENU_COLOR_BORDER].hex);

    /* Shell */
    printf("/* Default shell to run chord commands with. */\n");
    printf("static const char* shell = \"%s\";\n", menu->shell);

    /* Font */
    printf("/* Pango font description i.e. 'Noto Mono, M+ 1c, ..., 16'. */\n");
    printf("static const char* font = \"%s\";\n", menu->font);

    /* Implicit array keys */
    printf("/* Keys to use for chord arrays */\n");
    printf("static const char* implicitArrayKeys = \"%s\";\n", menu->implicitArrayKeys);

    /* Wrap command */
    printf("/* Command wrapper prefix. Set to NULL or \"\" to disable. Examples: \"uwsm app --\", \"firefox\", etc. */\n");
    if (stringIsEmpty(&menu->wrapCmd))
    {
        printf("static const char* wrapCmd = NULL;\n");
    }
    else
    {
        printf("static const char* wrapCmd = \"");
        writeEscString(&menu->wrapCmd);
        printf("\";\n");
    }

    printf("\n");
}

static void
writeKeyChordsDefines(void)
{
    printf(
        "/* Builtin key chords */\n"
        "#define ARRAY(T, _len, ...)                  \\\n"
        "    (Array)                                  \\\n"
        "    {                                        \\\n"
        "        .data        = (T[]){ __VA_ARGS__ }, \\\n"
        "        .length      = (_len),               \\\n"
        "        .capacity    = (_len),               \\\n"
        "        .elementSize = sizeof(T)             \\\n"
        "    }\n"
        "#define EMPTY_ARRAY(T)           \\\n"
        "    (Array)                      \\\n"
        "    {                            \\\n"
        "        .data        = NULL,     \\\n"
        "        .length      = 0,        \\\n"
        "        .capacity    = 0,        \\\n"
        "        .elementSize = sizeof(T) \\\n"
        "    }\n"
        "#define STRING(_offset, _len)                                                                       \\\n"
        "    (String)                                                                                        \\\n"
        "    {                                                                                               \\\n"
        "        .parts  = ARRAY(StringPart, 1, { .source = BUILTIN_SOURCE + (_offset), .length = (_len) }), \\\n"
        "        .length = (_len)                                                                            \\\n"
        "    }\n"
        "#define EMPTY_STRING (String){         \\\n"
        "    .parts  = EMPTY_ARRAY(StringPart), \\\n"
        "    .length = 0                        \\\n"
        "}\n"
        "#define KEY_CHORD(_key, _desc, _cmd, _before, _after, _wrap_cmd, _flags, _chords) \\\n"
        "    (KeyChord)                                                                    \\\n"
        "    {                                                                             \\\n"
        "        .key         = (_key),                                                    \\\n"
        "        .description = (_desc),                                                   \\\n"
        "        .command     = (_cmd),                                                    \\\n"
        "        .before      = (_before),                                                 \\\n"
        "        .after       = (_after),                                                  \\\n"
        "        .wrapCmd     = (_wrap_cmd),                                               \\\n"
        "        .flags       = (_flags),                                                  \\\n"
        "        .keyChords   = (_chords)                                                  \\\n"
        "    }\n"
        "#define KEY(_offset, _len, _mods, _special)   \\\n"
        "    (Key)                                     \\\n"
        "    {                                         \\\n"
        "        .repr    = STRING((_offset), (_len)), \\\n"
        "        .mods    = (_mods),                   \\\n"
        "        .special = (_special)                 \\\n"
        "    }\n"
        "\n");
}

static void
writeBuiltinSourceDeclaration(void)
{
    printf("static const char BUILTIN_SOURCE[] = ");
}

static void
writeBuiltinSourceImpl(const Array* arr)
{
    assert(arr);

    forEach(arr, KeyChord, keyChord)
    {
        writeEscString(&keyChord->key.repr);
        writeEscString(&keyChord->description);
        writeEscString(&keyChord->command);
        writeEscString(&keyChord->before);
        writeEscString(&keyChord->after);
        writeEscString(&keyChord->wrapCmd);
        if (!arrayIsEmpty(&keyChord->keyChords)) writeBuiltinSourceImpl(&keyChord->keyChords);
    }
}

static void
writeBuiltinSource(const Array* arr)
{
    assert(arr);

    printf("\"");
    writeBuiltinSourceImpl(arr);
    printf("\";\n\n");
}

static void
writeKeyChordsDeclaration(void)
{
    printf("static Array builtinKeyChords =\n    ");
}

static void
writeModifier(const Modifier mods, int indent)
{
    if (!modifierHasAnyActive(mods))
    {
        printf("MOD_NONE,");
    }
    else
    {
        int count = modifierCount(mods);
        if (modifierIsActive(mods, MOD_CTRL))
            printf("MOD_CTRL%s", getSeparator(&count, " | ", ","));
        if (modifierIsActive(mods, MOD_META))
            printf("MOD_META%s", getSeparator(&count, " | ", ","));
        if (modifierIsActive(mods, MOD_HYPER))
            printf("MOD_HYPER%s", getSeparator(&count, " | ", ","));
        if (modifierIsActive(mods, MOD_SHIFT))
            printf("MOD_SHIFT%s", getSeparator(&count, " | ", ","));
    }
    printf(" ");
}

static void
writeSpecialKey(SpecialKey special)
{
    printf("%s", specialKeyGetLiteral(special));
}

static void
writeOffsetAndLength(const String* string, const char* rest)
{
    assert(string), assert(rest);
    printf("%zu, %zu", offset, string->length);
    if (rest) printf("%s", rest);
    offset += string->length;
}

static void
writeKey(const Key* key, int indent)
{
    assert(key);

    writeNewlineWithIndent(indent);
    printf("KEY(");
    writeOffsetAndLength(&key->repr, ", ");
    writeModifier(key->mods, indent);
    writeSpecialKey(key->special);
    printf("),");
}

static void
writeString(const String* string, int indent)
{
    assert(string);

    writeNewlineWithIndent(indent);
    if (stringIsEmpty(string))
    {
        printf("EMPTY_STRING,");
    }
    else
    {
        printf("STRING(");
        writeOffsetAndLength(string, "),");
    }
}

static void
writeChordFlag(const ChordFlag flags, int indent)
{
    writeNewlineWithIndent(indent);
    if (!chordFlagHasAnyActive(flags))
    {
        printf("FLAG_NONE,");
    }
    else
    {
        int count = chordFlagCount(flags);
        if (chordFlagIsActive(flags, FLAG_KEEP))
            printf("FLAG_KEEP%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_CLOSE))
            printf("FLAG_CLOSE%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_INHERIT))
            printf("FLAG_INHERIT%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_IGNORE))
            printf("FLAG_IGNORE%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_IGNORE_SORT))
            printf("FLAG_IGNORE_SORT%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_UNHOOK))
            printf("FLAG_UNHOOK%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_DEFLAG))
            printf("FLAG_DEFLAG%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_NO_BEFORE))
            printf("FLAG_NO_BEFORE%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_NO_AFTER))
            printf("FLAG_NO_AFTER%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_WRITE))
            printf("FLAG_WRITE%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_EXECUTE))
            printf("FLAG_EXECUTE%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_SYNC_COMMAND))
            printf("FLAG_SYNC_COMMAND%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_SYNC_BEFORE))
            printf("FLAG_SYNC_BEFORE%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_SYNC_AFTER))
            printf("FLAG_SYNC_AFTER%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_UNWRAP))
            printf("FLAG_UNWRAP%s", getSeparator(&count, " | ", ","));
    }
}

static void
writePrefix(const Array* arr, int indent)
{
    assert(arr);

    writeNewlineWithIndent(indent);
    if (!arrayIsEmpty(arr))
    {
        writeKeyChords(arr, indent);
    }
    else
    {
        printf("EMPTY_ARRAY(KeyChord)");
    }
}

static void
writeChord(const KeyChord* keyChord, int indent)
{
    assert(keyChord);

    writeNewlineWithIndent(indent);
    printf("KEY_CHORD(");
    writeKey(&keyChord->key, indent + 1);
    writeString(&keyChord->description, indent + 1);
    writeString(&keyChord->command, indent + 1);
    writeString(&keyChord->before, indent + 1);
    writeString(&keyChord->after, indent + 1);
    writeString(&keyChord->wrapCmd, indent + 1);
    writeChordFlag(keyChord->flags, indent + 1);
    writePrefix(&keyChord->keyChords, indent + 1);
    printf(")");
}

static void
writeKeyChords(const Array* arr, int indent)
{
    assert(arr);

    printf("ARRAY(");
    writeNewlineWithIndent(indent + 1);
    printf("KeyChord,");
    writeNewlineWithIndent(indent + 1);
    printf("%zu,", arr->length);
    forEach(arr, KeyChord, keyChord)
    {
        writeChord(keyChord, indent + 1);
        if (iter.index < arr->length - 1) printf(",");
    }
    printf(")");
}

void
writeConfigHeaderFile(const Array* keyChords, const Menu* menu)
{
    assert(keyChords);
    assert(menu);

    writeConfigHeader();
    writeConfigVariables(menu);
    writeKeyChordsDefines();
    writeBuiltinSourceDeclaration();
    writeBuiltinSource(keyChords);
    writeKeyChordsDeclaration();
    writeKeyChords(keyChords, 1);
    printf(";\n\n#endif /* WK_CONFIG_CONFIG_H_ */\n");
}
