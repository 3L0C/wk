#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

/* common includes */
#include "common/key_chord.h"
#include "common/util.h"

/* local includes */
#include "writer.h"

static void writeKeyChords(const KeyChord* keyChords, int indent);

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
        "/* state, mods, specials,\n"
        " * key, description, hint,\n"
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
writeChordMods(const KeyChordMods* mods)
{
    printf(
        "MAKE_MODS(%s, %s, %s, %s), ",
        (mods->ctrl ? "true" : "false"),
        (mods->alt ? "true" : "false"),
        (mods->hyper ? "true" : "false"),
        (mods->shift ? "true" : "false")
    );
}

static void
writeChordSpecial(SpecialKey special)
{
    printf("%s, ", getSpecialKeyRepr(special));
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
writeChordFlags(const KeyChordFlags* flags, int indent)
{
    printf("MAKE_FLAGS(\n");
    printf("%*s", indent * 4, " ");
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
    printf("%*s", indent * 4, " ");
    printf("    %-5s, %-5s, %-5s, %-5s, %-5s, %-5s\n",
        (flags->noafter ? "true" : "false"),
        (flags->write ? "true" : "false"),
        (flags->execute ? "true" : "false"),
        (flags->syncCommand ? "true" : "false"),
        (flags->syncBefore ? "true" : "false"),
        (flags->syncAfter ? "true" : "false")
    );
    printf("%*s", indent * 4, " ");
    printf("), ");
}

static void
writeChord(const KeyChord* keyChord, int indent)
{
    assert(keyChord);

    printf("%*s", indent * 4, " ");
    printf("KEY_CHORD_STATE_NOT_NULL, ");
    writeChordMods(&keyChord->mods);

    printf("\n%*s", indent * 4, " ");
    writeChordSpecial(keyChord->special);
    writeEscString(keyChord->key);
    writeEscString(keyChord->description);
    writeEscString(keyChord->hint);

    /* command */
    printf("\n%*s", indent * 4, " ");
    writeEscString(keyChord->command);

    /* before */
    printf("\n%*s", indent * 4, " ");
    writeEscString(keyChord->before);

    /* after */
    printf("\n%*s", indent * 4, " ");
    writeEscString(keyChord->after);

    /* flags */
    printf("\n%*s", indent * 4, " ");
    writeChordFlags(&keyChord->flags, indent);

    /* prefix */
    if (keyChord->keyChords)
    {
        printf("\n%*s", indent * 4, " ");
        printf("PREFIX(\n");
        writeKeyChords(keyChord->keyChords, indent + 1);
        printf("%*s)\n", indent * 4, " ");
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
