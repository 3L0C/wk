#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

/* common includes */
#include "common/array.h"
#include "common/common.h"
#include "common/key_chord.h"
#include "common/string.h"

static size_t offset = 0;

static void writeKeyChords(const Array* keyChords, int indent);

static void
writeNewlineWithIndent(int indent)
{
    printf("\n%*s", indent * 4, " ");
}

static void
writeChordsHeader(void)
{
    printf(
        "#ifndef WK_CONFIG_KEY_CHORDS_H_\n"
        "#define WK_CONFIG_KEY_CHORDS_H_\n"
        "\n"
        "#include <stddef.h>\n"
        "\n"
        "/* common includes */\n"
        "#include \"src/common/array.h\"\n"
        "#include \"src/common/key_chord.h\"\n"
        "#include \"src/common/string.h\"\n"
        "\n"
        "#define ARRAY(T, len, ...) (Array){ \\\n"
        "    .data = (T[]){ __VA_ARGS__ }, \\\n"
        "    .length = (len), \\\n"
        "    .capacity = (len), \\\n"
        "    .elementSize = sizeof(T) \\\n"
        "}\n"
        "#define EMPTY_ARRAY(T) (Array){ \\\n"
        "    .data = NULL, \\\n"
        "    .length = 0, \\\n"
        "    .capacity = 0, \\\n"
        "    .elementSize = sizeof(T) \\\n"
        "}\n"
        "#define STRING(offset, len) (String){ \\\n"
        "    .parts = ARRAY(StringPart, (len), { .source = BUILTIN_SOURCE + (offset), .length = (len) }), \\\n"
        "    .length = (len) \\\n"
        "}\n"
        "#define EMPTY_STRING (String){ \\\n"
        "    .parts = EMPTY_ARRAY(StringPart), \\\n"
        "    .length = 0 \\\n"
        "}\n"
        "#define KEY_CHORD(_key, desc, cmd, _before, _after, _flags, chords) \\\n"
        "    (KeyChord){ \\\n"
        "        .key         = (_key), \\\n"
        "        .description = (desc), \\\n"
        "        .command     = (cmd), \\\n"
        "        .before      = (_before), \\\n"
        "        .after       = (_after), \\\n"
        "        .flags       = (_flags), \\\n"
        "        .keyChords   = (chords) \\\n"
        "    }\n"
        "#define KEY(offset, len, _mods, _special) \\\n"
        "    (Key){ \\\n"
        "        .repr    = STRING((offset), (len)), \\\n"
        "        .mods    = (_mods), \\\n"
        "        .special = (_special) \\\n"
        "    }\n"
        "\n"
        "static const char BUILTIN_SOURCE[] = "
    );
}

static void
writeEscString(const String* string)
{
    if (!string) return;
    if (stringIsEmpty(string)) return;

    StringIterator iter = stringIteratorMake(string);
    char c = '\0';
    while ((c = stringIteratorNext(&iter)) != '\0')
    {
        switch (c)
        {
        case '\\': printf("\\"); break;
        case '\"': printf("\\\""); break;
        case '\n': printf("\\\n"); break;
        default: printf("%c", c); break;
        }
    }
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
writetKeyChordsDeclaration(void)
{
    printf("static Array builtinKeyChords = ");
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
        if (modifierIsActive(mods, MOD_CTRL)) printf("MOD_CTRL%c", getSeparator(&count, '|', ','));
        if (modifierIsActive(mods, MOD_META)) printf("MOD_META%c", getSeparator(&count, '|', ','));
        if (modifierIsActive(mods, MOD_HYPER)) printf("MOD_HYPER%c", getSeparator(&count, '|', ','));
        if (modifierIsActive(mods, MOD_SHIFT)) printf("MOD_SHIFT%c", getSeparator(&count, '|', ','));
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
        if (chordFlagIsActive(flags, FLAG_KEEP)) printf("FLAG_KEEP%c", getSeparator(&count, '|', ','));
        if (chordFlagIsActive(flags, FLAG_CLOSE)) printf("FLAG_CLOSE%c", getSeparator(&count, '|', ','));
        if (chordFlagIsActive(flags, FLAG_INHERIT)) printf("FLAG_INHERIT%c", getSeparator(&count, '|', ','));
        if (chordFlagIsActive(flags, FLAG_IGNORE)) printf("FLAG_IGNORE%c", getSeparator(&count, '|', ','));
        if (chordFlagIsActive(flags, FLAG_IGNORE_SORT)) printf("FLAG_IGNORE_SORT%c", getSeparator(&count, '|', ','));
        if (chordFlagIsActive(flags, FLAG_UNHOOK)) printf("FLAG_UNHOOK%c", getSeparator(&count, '|', ','));
        if (chordFlagIsActive(flags, FLAG_DEFLAG)) printf("FLAG_DEFLAG%c", getSeparator(&count, '|', ','));
        if (chordFlagIsActive(flags, FLAG_NO_BEFORE)) printf("FLAG_NO_BEFORE%c", getSeparator(&count, '|', ','));
        if (chordFlagIsActive(flags, FLAG_NO_AFTER)) printf("FLAG_NO_AFTER%c", getSeparator(&count, '|', ','));
        if (chordFlagIsActive(flags, FLAG_WRITE)) printf("FLAG_WRITE%c", getSeparator(&count, '|', ','));
        if (chordFlagIsActive(flags, FLAG_EXECUTE)) printf("FLAG_EXECUTE%c", getSeparator(&count, '|', ','));
        if (chordFlagIsActive(flags, FLAG_SYNC_COMMAND)) printf("FLAG_SYNC_COMMAND%c", getSeparator(&count, '|', ','));
        if (chordFlagIsActive(flags, FLAG_SYNC_BEFORE)) printf("FLAG_SYNC_BEFORE%c", getSeparator(&count, '|', ','));
        if (chordFlagIsActive(flags, FLAG_SYNC_AFTER)) printf("FLAG_SYNC_AFTER%c", getSeparator(&count, '|', ','));
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
    writeChordFlag(keyChord->flags, indent + 1);
    writePrefix(&keyChord->keyChords, indent + 1);
    writeNewlineWithIndent(indent);
    printf(")");
}

static void
writeKeyChords(const Array* arr, int indent)
{
    assert(arr);

    printf("ARRAY(KeyChord, %zu,", arr->length);
    forEach(arr, KeyChord, keyChord)
    {
        writeChord(keyChord, indent + 1);
        if (iter.index < arr->length - 1) printf(",");
    }
    writeNewlineWithIndent(indent);
    printf(")");
}

static void
writeChordsFooter(void)
{
    printf(
        ";\n"
        "\n"
        "#endif /* WK_CONFIG_KEY_CHORDS_H_ */\n"
    );
}

void
writeBuiltinKeyChordsHeaderFile(const Array* keyChords)
{
    assert(keyChords);

    writeChordsHeader();
    writeBuiltinSource(keyChords);
    writetKeyChordsDeclaration();
    writeKeyChords(keyChords, 0);
    writeChordsFooter();
}
