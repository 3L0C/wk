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
        "#include \"src/common/key_chord.h\"\n"
        "\n"
        "/* state,\n"
        " * KEY(\n"
        " *     mods,\n"
        " *     special,\n"
        " *     key, key_len\n"
        " * ),\n"
        " * description,\n"
        " * command\n"
        " * before\n"
        " * after\n"
        " * flags, chords\n"
        " */\n"
        "KeyChord builtinKeyChords[] = {\n"
    );
}

static void
writeChordsFooter(void)
{
    printf(
        "};\n"
        "\n"
        "#endif /* WK_CONFIG_KEY_CHORDS_H_ */\n"
    );
}

static void
writeChordMods(const Modifiers* mods, int indent)
{
    assert(mods);

    printf(".mods = {");
    writeNewlineWithIndent(indent + 1);
    printf(
        ".ctrl = %-5s, .alt = %-5s, .hyper = %-5s, .shift = %-5s",
        (mods->ctrl ? "true" : "false"),
        (mods->alt ? "true" : "false"),
        (mods->hyper ? "true" : "false"),
        (mods->shift ? "true" : "false")
    );
    writeNewlineWithIndent(indent);
    printf("},");
}

static void
writeChordSpecial(SpecialKey special)
{
    printf("    .special = %s,", getSpecialKeyLiteral(special));
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
writeEscStringWithIndent(const char* member, const char* text, int indent)
{
    writeIndent(indent);
    printf(".%s = ", member);
    writeEscString(text);
}

static void
writeEscStringWithIndentAndNewline(const char* member, const char* text, int indent)
{
    printf("\n");
    writeEscStringWithIndent(member, text, indent);
}

static void
writeChordKey(const Key* key, int indent)
{
    assert(key);

    writeNewlineWithIndent(indent);
    printf(".key = {");
    writeNewlineWithIndent(indent + 1);
    writeChordMods(&key->mods, indent + 1);
    writeNewlineWithIndent(indent);
    writeChordSpecial(key->special);
    printf("\n    ");
    writeEscStringWithIndent("repr", key->repr, indent);
    printf(".len = %lu", strlen(key->repr));
    writeNewlineWithIndent(indent);
    printf("},\n");
}

static void
writeChordFlags(const ChordFlags* flags, int indent)
{
    assert(flags);

    printf(".flags = {\n");
    writeIndent(indent);
    printf(
        "    %-5s, %-5s, %-5s, %-5s, %-5s, %-5s, %-5s,\n",
        (flags->keep ? "true" : "false"),
        (flags->close ? "true" : "false"),
        (flags->inherit ? "true" : "false"),
        (flags->ignore ? "true" : "false"),
        (flags->ignoreSort ? "true" : "false"),
        (flags->unhook ? "true" : "false"),
        (flags->deflag ? "true" : "false")
    );
    writeIndent(indent);
    printf("    %-5s, %-5s, %-5s, %-5s, %-5s, %-5s, %-5s\n",
        (flags->nobefore ? "true" : "false"),
        (flags->noafter ? "true" : "false"),
        (flags->write ? "true" : "false"),
        (flags->execute ? "true" : "false"),
        (flags->syncCommand ? "true" : "false"),
        (flags->syncBefore ? "true" : "false"),
        (flags->syncAfter ? "true" : "false")
    );
    writeIndent(indent);
    printf("}, ");
}

static void
writeChord(const KeyChord* keyChord, int indent)
{
    assert(keyChord);

    printf("%*s", indent * 4, " ");
    printf(".state = KEY_CHORD_STATE_NOT_NULL, ");
    writeChordKey(&keyChord->key, indent);
    writeEscStringWithIndent("description", keyChord->description, indent);

    /* command */
    writeEscStringWithIndentAndNewline("command", keyChord->command, indent);

    /* before */
    writeEscStringWithIndentAndNewline("before", keyChord->before, indent);

    /* after */
    writeEscStringWithIndentAndNewline("after", keyChord->after, indent);

    /* flags */
    writeNewlineWithIndent(indent);
    writeChordFlags(&keyChord->flags, indent);

    /* prefix */
    if (keyChord->keyChords)
    {
        writeNewlineWithIndent(indent);
        printf(".keyChords = (KeyChord[]){\n");
        writeKeyChords(keyChord->keyChords, indent + 1);
        writeIndent(indent);
        printf("}\n");
    }
    else
    {
        printf(".keyChords = NULL\n");
    }
}

static void
writeNullKeyChord(int indent)
{
    writeIndent(indent);
    printf("{ .state = KEY_CHORD_STATE_IS_NULL }\n");
}

static void
writeKeyChords(const KeyChord* keyChords, int indent)
{
    assert(keyChords);

    size_t count = countKeyChords(keyChords);
    for (size_t i = 0; i < count; i++)
    {
        writeIndent(indent);
        printf("{\n");
        writeChord(&keyChords[i], indent + 1);
        writeIndent(indent);
        printf("},\n");
    }

    writeNullKeyChord(indent);
}

void
writeBuiltinKeyChordsHeaderFile(const KeyChord* keyChords)
{
    assert(keyChords);

    writeChordsHeader();
    writeKeyChords(keyChords, 1);
    writeChordsFooter();
}
