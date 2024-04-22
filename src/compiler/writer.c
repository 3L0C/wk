#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* common includes */
#include "common/key_chord.h"

/* local includes */
#include "writer.h"

static void writeKeyChords(const KeyChord* keyChords, int indent);

static void
writeIndent(int indent)
{
    printf("%*s", indent * 4, " ");
}

static void
writeIndentWithNewline(int indent)
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
        "#include \"src/common/key_chord.h\"\n"
        "\n"
        "/* state,\n"
        " * KEY(\n"
        " *     mods,\n"
        " *     special,\n"
        " *     key, key_len\n"
        " * ),\n"
        " * description,\n"
        " * hint,\n"
        " * command\n"
        " * before\n"
        " * after\n"
        " * flags, chords\n"
        " */\n"
        "KeyChord builtinKeyChords[] = KEY_CHORDS(\n"
    );
}

static void
writeChordsFooter(void)
{
    printf(
        ");\n"
        "\n"
        "#endif /* WK_CONFIG_KEY_CHORDS_H_ */\n"
    );
}

static void
writeChordMods(const Modifiers* mods)
{
    assert(mods);

    printf(
        "    %-5s, %-5s, %-5s, %-5s,",
        (mods->ctrl ? "true" : "false"),
        (mods->alt ? "true" : "false"),
        (mods->hyper ? "true" : "false"),
        (mods->shift ? "true" : "false")
    );
}

static void
writeChordSpecial(SpecialKey special)
{
    printf("    %s,", getSpecialKeyLiteral(special));
}

static void
writeEscString(const char* text)
{
    if (!text)
    {
        printf("NULL, ");
        return;
    }

    printf("\"");
    const char* current = text;
    while (*current != '\0')
    {
        switch (*current)
        {
        case '\\': printf("\\"); break;
        case '\"': printf("\\\""); break;
        case '\n': printf("\\\n"); break;
        default: printf("%c", *current); break;
        }
        current++;
    }
    printf("\", ");
}

static void
writeEscStringWithIndent(const char* text, int indent)
{
    writeIndent(indent);
    writeEscString(text);
}

static void
writeEscStringWithIndentAndNewline(const char* text, int indent)
{
    printf("\n");
    writeEscStringWithIndent(text, indent);
}

static void
writeChordKey(const Key* key, int indent)
{
    assert(key);

    writeIndentWithNewline(indent);
    printf("MAKE_KEY(");
    writeIndentWithNewline(indent);
    writeChordMods(&key->mods);
    writeIndentWithNewline(indent);
    writeChordSpecial(key->special);
    printf("\n    ");
    writeEscStringWithIndent(key->repr, indent);
    printf("%lu", strlen(key->repr));
    writeIndentWithNewline(indent);
    printf("),\n");
}

static void
writeChordFlags(const ChordFlags* flags, int indent)
{
    assert(flags);

    printf("MAKE_FLAGS(\n");
    writeIndent(indent);
    printf(
        "    %-5s, %-5s, %-5s, %-5s, %-5s, %-5s, %-5s,\n",
        (flags->keep ? "true" : "false"),
        (flags->close ? "true" : "false"),
        (flags->inherit ? "true" : "false"),
        (flags->ignore ? "true" : "false"),
        (flags->unhook ? "true" : "false"),
        (flags->deflag ? "true" : "false"),
        (flags->nobefore ? "true" : "false")
    );
    writeIndent(indent);
    printf("    %-5s, %-5s, %-5s, %-5s, %-5s, %-5s\n",
        (flags->noafter ? "true" : "false"),
        (flags->write ? "true" : "false"),
        (flags->execute ? "true" : "false"),
        (flags->syncCommand ? "true" : "false"),
        (flags->syncBefore ? "true" : "false"),
        (flags->syncAfter ? "true" : "false")
    );
    writeIndent(indent);
    printf("), ");
}

static void
writeChord(const KeyChord* keyChord, int indent)
{
    assert(keyChord);

    printf("%*s", indent * 4, " ");
    printf("KEY_CHORD_STATE_NOT_NULL, ");
    writeChordKey(&keyChord->key, indent);
    writeEscStringWithIndent(keyChord->description, indent);
    writeEscString(keyChord->hint);

    /* command */
    writeEscStringWithIndentAndNewline(keyChord->command, indent);

    /* before */
    writeEscStringWithIndentAndNewline(keyChord->before, indent);

    /* after */
    writeEscStringWithIndentAndNewline(keyChord->after, indent);

    /* flags */
    writeIndentWithNewline(indent);
    writeChordFlags(&keyChord->flags, indent);

    /* prefix */
    if (keyChord->keyChords)
    {
        writeIndentWithNewline(indent);
        printf("PREFIX(\n");
        writeKeyChords(keyChord->keyChords, indent + 1);
        writeIndent(indent);
        printf(")\n");
    }
    else
    {
        printf("NULL\n");
    }
}

static void
writeKeyChords(const KeyChord* keyChords, int indent)
{
    assert(keyChords);

    size_t count = countKeyChords(keyChords);
    for (size_t i = 0; i < count; i++)
    {
        printf("%*s", indent * 4, " ");
        printf("{\n");
        writeChord(&keyChords[i], indent + 1);
        printf("%*s}", indent * 4, " ");
        if (i + 1 != count)
        {
            printf(",");
        }
        printf("\n");
    }
}

void
writeBuiltinKeyChordsHeaderFile(const KeyChord* keyChords)
{
    assert(keyChords);

    writeChordsHeader();
    writeKeyChords(keyChords, 1);
    writeChordsFooter();
}
