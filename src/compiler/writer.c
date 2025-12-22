#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

/* common includes */
#include "common/common.h"
#include "common/key_chord.h"
#include "common/menu.h"
#include "common/property.h"
#include "common/span.h"
#include "common/string.h"

static void writeKeyChords(const Span* keyChords, int indent);

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
        "#include \"src/common/key_chord.h\"\n"
        "#include \"src/common/menu.h\"\n"
        "#include \"src/common/span.h\"\n"
        "#include \"src/common/string.h\"\n"
        "\n");
}

static void
writeEscString(const String* str)
{
    if (!str || stringIsEmpty(str)) return;

    for (size_t i = 0; i < str->length; i++)
    {
        char c = str->data[i];
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
writeEscCString(const char* str)
{
    if (!str) return;

    const char* c = str;
    while (*c != '\0')
    {
        switch (*c)
        {
        case '\\': printf("\\\\"); break;
        case '\"': printf("\\\""); break;
        case '\n': printf("\\\n"); break;
        default: printf("%c", *c); break;
        }
        c++;
    }
}

static void
writeEscStringQuoted(const String* str)
{
    printf("\"");
    if (str && !stringIsEmpty(str)) writeEscString(str);
    printf("\"");
}

static void
writeConfigVariables(const Menu* menu)
{
    assert(menu);

    /* Delimiter */
    printf("/* Delimiter when displaying chords. */\n");
    printf("static const char* delimiter = \"");
    writeEscCString(menu->delimiter);
    printf("\";\n");

    /* Delay */
    printf("/* Delay between last keypress and first time displaying the menu. Value in milliseconds. */\n");
    printf("static uint32_t delay = %u;\n", menu->delay);

    /* Keep delay */
    printf("/* Delay in milliseconds after ungrab before command execution for +keep chords. */\n");
    printf("static uint32_t keepDelay = %u;\n", menu->keepDelay);

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
    printf("    [FOREGROUND_COLOR_KEY]       = \"");
    writeEscCString(menu->colors[MENU_COLOR_KEY].hex);
    printf("\",\n");
    printf("    [FOREGROUND_COLOR_DELIMITER] = \"");
    writeEscCString(menu->colors[MENU_COLOR_DELIMITER].hex);
    printf("\",\n");
    printf("    [FOREGROUND_COLOR_PREFIX]    = \"");
    writeEscCString(menu->colors[MENU_COLOR_PREFIX].hex);
    printf("\",\n");
    printf("    [FOREGROUND_COLOR_CHORD]     = \"");
    writeEscCString(menu->colors[MENU_COLOR_CHORD].hex);
    printf("\",\n");
    printf("    [FOREGROUND_COLOR_TITLE]     = \"");
    writeEscCString(menu->colors[MENU_COLOR_TITLE].hex);
    printf("\",\n");
    printf("    [FOREGROUND_COLOR_GOTO]      = \"");
    writeEscCString(menu->colors[MENU_COLOR_GOTO].hex);
    printf("\",\n");
    printf("};\n");

    /* Background color */
    printf("/* Menu background color */\n");
    printf("static const char* background = \"");
    writeEscCString(menu->colors[MENU_COLOR_BACKGROUND].hex);
    printf("\";\n");

    /* Border color */
    printf("/* Menu border color */\n");
    printf("static const char* border = \"");
    writeEscCString(menu->colors[MENU_COLOR_BORDER].hex);
    printf("\";\n");

    /* Shell */
    printf("/* Default shell to run chord commands with. */\n");
    printf("static const char* shell = \"");
    writeEscCString(menu->shell);
    printf("\";\n");

    /* Font */
    printf("/* Pango font description i.e. 'Noto Mono, M+ 1c, ..., 16'. */\n");
    printf("static const char* font = \"");
    writeEscCString(menu->font);
    printf("\";\n");

    /* Title Font */
    printf("/* Pango font description i.e. 'Inter Nerd Font, M+ 1c, ..., 16'. */\n");
    printf("static const char* titleFont = \"");
    writeEscCString(menu->titleFont);
    printf("\";\n");

    /* Implicit array keys */
    printf("/* Keys to use for chord arrays */\n");
    printf("static const char* implicitArrayKeys = \"");
    writeEscCString(menu->implicitArrayKeys);
    printf("\";\n");

    /* Wrap command */
    printf("/* Command wrapper prefix. Set to NULL or \"\" to disable. Examples: \"uwsm app --\", \"firefox\", etc. */\n");
    if (menu->wrapCmd == NULL || menu->wrapCmd[0] == '\0')
    {
        printf("static const char* wrapCmd = NULL;\n");
    }
    else
    {
        printf("static const char* wrapCmd = \"");
        writeEscCString(menu->wrapCmd);
        printf("\";\n");
    }

    printf("\n");
}

static void
writeKeyChordsDefines(void)
{
    printf(
        "/* Builtin key chords */\n"
        "#define SPAN_STATIC(T, _count, ...)    \\\n"
        "    (Span)                             \\\n"
        "    {                                  \\\n"
        "        .data  = (T[]){ __VA_ARGS__ }, \\\n"
        "        .count = (_count)              \\\n"
        "    }\n"
        "#define STRING(_str)               \\\n"
        "    (String){                      \\\n"
        "        .data   = (_str),          \\\n"
        "        .length = sizeof(_str) - 1 \\\n"
        "    }\n"
        "#define STRING_EMPTY (String){ .data = \"\", .length = 0 }\n"
        "#define PROPERTY_STRING(_str)                 \\\n"
        "    (Property)                                \\\n"
        "    {                                         \\\n"
        "        .type  = PROP_TYPE_STRING,            \\\n"
        "        .value = {.as_string = STRING(_str) } \\\n"
        "    }\n"
        "#define PROPERTY_STRING_EMPTY                 \\\n"
        "    (Property)                                \\\n"
        "    {                                         \\\n"
        "        .type  = PROP_TYPE_STRING,            \\\n"
        "        .value = {.as_string = STRING_EMPTY } \\\n"
        "    }\n"
        "#define PROPERTIES(...) __VA_ARGS__\n"
        "#define PROPERTIES_EMPTY\n"
        "#define KEY_CHORD(_key, _props, _flags, _chords) \\\n"
        "    (KeyChord)                                   \\\n"
        "    {                                            \\\n"
        "        .key       = (_key),                     \\\n"
        "        .props     = { _props },                 \\\n"
        "        .flags     = (_flags),                   \\\n"
        "        .keyChords = (_chords)                   \\\n"
        "    }\n"
        "#define KEY(_repr, _mods, _special) \\\n"
        "    (Key)                           \\\n"
        "    {                               \\\n"
        "        .repr    = STRING(_repr),   \\\n"
        "        .mods    = (_mods),         \\\n"
        "        .special = (_special)       \\\n"
        "    }\n"
        "\n");
}

static void
writeKeyChordsDeclaration(void)
{
    printf("static Span builtinKeyChords =\n    ");
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
        if (modifierIsActive(mods, MOD_CTRL)) printf("MOD_CTRL%s", getSeparator(&count, " | ", ","));
        if (modifierIsActive(mods, MOD_META)) printf("MOD_META%s", getSeparator(&count, " | ", ","));
        if (modifierIsActive(mods, MOD_HYPER)) printf("MOD_HYPER%s", getSeparator(&count, " | ", ","));
        if (modifierIsActive(mods, MOD_SHIFT)) printf("MOD_SHIFT%s", getSeparator(&count, " | ", ","));
    }
    printf(" ");
}

static void
writeSpecialKey(SpecialKey special)
{
    printf("%s", specialKeyLiteral(special));
}

static void
writeKey(const Key* key, int indent)
{
    assert(key);

    writeNewlineWithIndent(indent);
    printf("KEY(");
    writeEscStringQuoted(&key->repr);
    printf(", ");
    writeModifier(key->mods, indent);
    writeSpecialKey(key->special);
    printf("),");
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
        if (chordFlagIsActive(flags, FLAG_KEEP)) printf("FLAG_KEEP%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_CLOSE)) printf("FLAG_CLOSE%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_INHERIT)) printf("FLAG_INHERIT%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_IGNORE)) printf("FLAG_IGNORE%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_UNHOOK)) printf("FLAG_UNHOOK%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_DEFLAG)) printf("FLAG_DEFLAG%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_NO_BEFORE)) printf("FLAG_NO_BEFORE%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_NO_AFTER)) printf("FLAG_NO_AFTER%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_WRITE)) printf("FLAG_WRITE%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_EXECUTE)) printf("FLAG_EXECUTE%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_SYNC_COMMAND)) printf("FLAG_SYNC_COMMAND%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_SYNC_BEFORE)) printf("FLAG_SYNC_BEFORE%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_SYNC_AFTER)) printf("FLAG_SYNC_AFTER%s", getSeparator(&count, " | ", ","));
        if (chordFlagIsActive(flags, FLAG_UNWRAP)) printf("FLAG_UNWRAP%s", getSeparator(&count, " | ", ","));
    }
}

static void
writePrefix(const Span* span, int indent)
{
    assert(span);

    writeNewlineWithIndent(indent);
    if (span->count != 0)
    {
        writeKeyChords(span, indent);
    }
    else
    {
        printf("SPAN_EMPTY");
    }
}

static bool
propertyIsEmpty(const Property* prop)
{
    assert(prop);

    switch (prop->type)
    {
    case PROP_TYPE_STRING:
        return stringIsEmpty(PROP_VAL(prop, as_string));
    case PROP_TYPE_NONE:
        return true;
    default:
        return true;
    }
}

static void
writePropertyDesignator(PropId id, const Property* prop, size_t maxNameLength)
{
    assert(prop);
    assert(id < KC_PROP_COUNT);

    const char* name    = propRepr(id);
    size_t      nameLen = strlen(name);

    /* Align the '=' sign by padding shorter property names */
    printf("[%s]%*s = ", name, (int)(maxNameLength - nameLen), "");

    switch (prop->type)
    {
    case PROP_TYPE_STRING:
        printf("PROPERTY_STRING(");
        writeEscStringQuoted(PROP_VAL(prop, as_string));
        printf(")");
        break;
    default:
        printf("PROPERTY_STRING_EMPTY");
        break;
    }
}

static void
writeChord(const KeyChord* keyChord, int indent)
{
    assert(keyChord);

    writeNewlineWithIndent(indent);
    printf("KEY_CHORD(");
    writeKey(&keyChord->key, indent + 1);

    /* Write properties (sparse - only non-empty properties) */
    writeNewlineWithIndent(indent + 1);

    /* Check if any properties are set and find max property name length for alignment */
    bool   hasProperties = false;
    size_t maxNameLength = 0;
    for (size_t i = 0; i < KC_PROP_COUNT; i++)
    {
        if (!propertyIsEmpty(&keyChord->props[i]))
        {
            hasProperties  = true;
            size_t nameLen = strlen(propRepr((PropId)i));
            if (nameLen > maxNameLength)
            {
                maxNameLength = nameLen;
            }
        }
    }

    if (hasProperties)
    {
        printf("PROPERTIES(");
        bool first = true;
        for (size_t i = 0; i < KC_PROP_COUNT; i++)
        {
            if (!propertyIsEmpty(&keyChord->props[i]))
            {
                if (!first)
                {
                    printf(",");
                }
                writeNewlineWithIndent(indent + 2);
                writePropertyDesignator((PropId)i, &keyChord->props[i], maxNameLength);
                first = false;
            }
        }
        printf("),");
    }
    else
    {
        printf("PROPERTIES_EMPTY,");
    }

    writeChordFlag(keyChord->flags, indent + 1);
    writePrefix(&keyChord->keyChords, indent + 1);
    printf(")");
}

static void
writeKeyChords(const Span* span, int indent)
{
    assert(span);

    if (span->count == 0)
    {
        printf("SPAN_EMPTY");
        return;
    }

    printf("SPAN_STATIC(");
    writeNewlineWithIndent(indent + 1);
    printf("KeyChord,");
    writeNewlineWithIndent(indent + 1);
    printf("%zu,", span->count);
    size_t index = 0;
    spanForEach(span, const KeyChord, keyChord)
    {
        writeChord(keyChord, indent + 1);
        if (index++ < span->count - 1) printf(",");
    }
    printf(")");
}

void
writeConfigHeaderFile(const Span* keyChords, const Menu* menu)
{
    assert(keyChords);
    assert(menu);

    writeConfigHeader();
    writeConfigVariables(menu);
    writeKeyChordsDefines();
    writeKeyChordsDeclaration();
    writeKeyChords(keyChords, 1);
    printf(";\n\n#endif /* WK_CONFIG_CONFIG_H_ */\n");
}
