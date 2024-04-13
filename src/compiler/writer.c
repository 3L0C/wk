#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>

/* common includes */
#include "common/types.h"

/* local includes */
#include "line.h"
#include "token.h"
#include "writer.h"

typedef enum
{
    WRITER_STATE_NORMAL,
    WRITER_STATE_IGNORING,
    WRITER_STATE_UPPER_FIRST,
    WRITER_STATE_LOWER_FIRST,
    WRITER_STATE_UPPER_ALL,
    WRITER_STATE_LOWER_ALL,
} WriterState;

static const char* delim;
static WriterState writerState;

static void writeChordLines(LineArray* lines, int indent);
static void writeChordRawString(TokenArray* array, Line* line);

static void
writeChordsHeader(void)
{
    printf(
        "#ifndef WK_CONFIG_KEY_CHORDS_H_\n"
        "#define WK_CONFIG_KEY_CHORDS_H_\n"
        "\n"
        "#include <stddef.h>\n"
        "\n"
        "#include \"src/common/types.h\"\n"
        "\n"
        "/* state, mods, specials,\n"
        " * key, description, hint,\n"
        " * command\n"
        " * before\n"
        " * after\n"
        " * flags, chords\n"
        " */\n"
        "WkKeyChord keyChords[] = KEY_CHORDS(\n"
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
writeChordMods(WkMods* mods)
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
writeChordSpecial(Token* token)
{
    switch (token->type)
    {
    /* Special key */
    case TOKEN_SPECIAL_LEFT:        printf("WK_SPECIAL_LEFT, ");       break;
    case TOKEN_SPECIAL_RIGHT:       printf("WK_SPECIAL_RIGHT, ");      break;
    case TOKEN_SPECIAL_UP:          printf("WK_SPECIAL_UP, ");         break;
    case TOKEN_SPECIAL_DOWN:        printf("WK_SPECIAL_DOWN, ");       break;
    case TOKEN_SPECIAL_TAB:         printf("WK_SPECIAL_TAB, ");        break;
    case TOKEN_SPECIAL_SPACE:       printf("WK_SPECIAL_SPACE, ");      break;
    case TOKEN_SPECIAL_RETURN:      printf("WK_SPECIAL_RETURN, ");     break;
    case TOKEN_SPECIAL_DELETE:      printf("WK_SPECIAL_DELETE, ");     break;
    case TOKEN_SPECIAL_ESCAPE:      printf("WK_SPECIAL_ESCAPE, ");     break;
    case TOKEN_SPECIAL_HOME:        printf("WK_SPECIAL_HOME, ");       break;
    case TOKEN_SPECIAL_PAGE_UP:     printf("WK_SPECIAL_PAGE_UP, ");    break;
    case TOKEN_SPECIAL_PAGE_DOWN:   printf("WK_SPECIAL_PAGE_DOWN, ");  break;
    case TOKEN_SPECIAL_END:         printf("WK_SPECIAL_END, ");        break;
    case TOKEN_SPECIAL_BEGIN:       printf("WK_SPECIAL_BEGIN, ");      break;
    /* Regular key */
    default: printf("WK_SPECIAL_NONE, "); break;
    }
}

static void
writeChordEscKey(Token* token)
{
    switch (token->start[0])
    {
    case '\\': printf("\\"); break;
    case '\"': printf("\\\""); break;
    default: printf("%.*s", (int)token->length, token->start); break;
    }
}

static void
writeChordKey(Token* token)
{
    printf("\"");
    writeChordEscKey(token);
    printf("\", ");
}

static size_t
rstrip(Token* token)
{
    assert(token);

    size_t index = token->length - 1;
    const char* lexeme = token->start;
    while (index && isspace(lexeme[index])) index--;
    return index + 1;
}

static void
writeChordEscString(Token* token, bool rstripFlag)
{
    assert(token);

    size_t length = rstripFlag ? rstrip(token) : token->length;

    for (size_t i = 0; i < length; i++)
    {
        char c = token->start[i];
        switch (c)
        {
        case '\\': printf("\\"); break;
        case '\"': printf("\\\""); break;
        case '\n': printf("\\\n"); break;
        default:
        {
            switch (writerState)
            {
            case WRITER_STATE_UPPER_FIRST:
            {
                writerState = WRITER_STATE_IGNORING;
                c = toupper(c); break;
            }
            case WRITER_STATE_LOWER_FIRST:
            {
                writerState = WRITER_STATE_IGNORING;
                c = tolower(c); break;
            }
            case WRITER_STATE_UPPER_ALL: c = toupper(c); break;
            case WRITER_STATE_LOWER_ALL: c = tolower(c); break;
            default: break;
            }
            printf("%c", c);
            break;
        }
        }
    }
}

static void
writeChordInterpWithState(Token* token, Line* line, WriterState state)
{
    WriterState oldState = writerState;
    writerState = state;
    writeChordRawString(&line->description, line);
    writerState = oldState;
}

static void
writeChordInterp(Token* token, Line* line)
{
    switch (token->type)
    {
    case TOKEN_INDEX:       printf("%d", line->index); break;
    case TOKEN_INDEX_ONE:   printf("%d", line->index + 1); break;
    case TOKEN_THIS_KEY:    writeChordEscKey(&line->key); break;
    case TOKEN_THIS_DESC:   writeChordRawString(&line->description, line); break;
    /* Cases where description is modified. */
    case TOKEN_THIS_DESC_UPPER_FIRST:
    {
        writeChordInterpWithState(token, line, WRITER_STATE_UPPER_FIRST);
        break;
    }
    case TOKEN_THIS_DESC_LOWER_FIRST:
    {
        writeChordInterpWithState(token, line, WRITER_STATE_LOWER_FIRST);
        break;
    }
    case TOKEN_THIS_DESC_UPPER_ALL:
    {
        writeChordInterpWithState(token, line, WRITER_STATE_UPPER_ALL);
        break;
    }
    case TOKEN_THIS_DESC_LOWER_ALL:
    {
        writeChordInterpWithState(token, line, WRITER_STATE_LOWER_ALL);
        break;
    }
    case TOKEN_COMM_INTERP: /* FALLTHROUGH */
    case TOKEN_DESC_INTERP: writeChordEscString(token, false); break;
    default: writeChordEscString(token, true); break;
    }
}

void
writeChordRawString(TokenArray* array, Line* line)
{
    for (size_t i = 0; i < array->count; i++)
    {
        writeChordInterp(&array->tokens[i], line);
    }
}

static void
writeChordString(TokenArray* array, Line* line)
{
    if (array->count == 0)
    {
        printf("NULL, ");
    }
    else
    {
        printf("\"");
        writeChordRawString(array, line);
        printf("\", ");
    }
}

static void
writeChordModStr(WkMods* mods)
{
    if (mods->ctrl)  printf("C-");
    if (mods->alt)   printf("A-");
    if (mods->hyper) printf("H-");
    if (mods->shift) printf("S-");
}

static void
writeChordHint(Line* line)
{
    printf("\"");
    writeChordModStr(&line->mods);
    writeChordEscKey(&line->key);
    printf("%s", delim);
    writeChordRawString(&line->description, line);
    printf("\",\n");
}

static void
writeChordFlags(WkFlags* flags)
{
    printf(
        "MAKE_FLAGS(%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s), ",
        (flags->keep ? "true" : "false"),
        (flags->close ? "true" : "false"),
        (flags->inherit ? "true" : "false"),
        (flags->ignore ? "true" : "false"),
        (flags->unhook ? "true" : "false"),
        (flags->deflag ? "true" : "false"),
        (flags->nobefore ? "true" : "false"),
        (flags->noafter ? "true" : "false"),
        (flags->write ? "true" : "false"),
        (flags->syncCommand ? "true" : "false"),
        (flags->beforeSync ? "true" : "false"),
        (flags->afterSync ? "true" : "false")
    );
}

static void
writeChordLine(Line* line, int indent)
{
    printf("%*s", indent * 4, " ");
    printf("WK_KEY_CHORD_STATE_NOT_NULL, ");
    writeChordMods(&line->mods);
    writeChordSpecial(&line->key);
    printf("\n%*s", indent * 4, " ");
    writeChordKey(&line->key);
    writeChordString(&line->description, line);
    writeChordHint(line);

    /* command */
    printf("%*s", indent * 4, " ");
    writeChordString(&line->command, line);

    /* before */
    printf("\n%*s", indent * 4, " ");
    writeChordString(&line->before, line);

    /* after */
    printf("\n%*s", indent * 4, " ");
    writeChordString(&line->after, line);

    /* flags */
    printf("\n%*s", indent * 4, " ");
    writeChordFlags(&line->flags);

    /* prefix */
    if (line->array.count != 0)
    {
        printf("\n%*s", indent * 4, " ");
        printf("PREFIX(\n");
        writeChordLines(&line->array, indent + 1);
        printf("%*s)\n", indent * 4, " ");
    }
    else
    {
        printf("NULL\n");
    }
}

static void
writeChordLines(LineArray* lines, int indent)
{
    for (size_t i = 0; i < lines->count; i++)
    {
        printf("%*s", indent * 4, " ");
        printf("{\n");
        writeChordLine(&lines->lines[i], indent + 1);
        printf("%*s}", indent * 4, " ");
        if (i + 1 != lines->count)
        {
            printf(",");
        }
        printf("\n");
    }
}

void
writeChords(LineArray* lines, const char* delimiter)
{
    assert(lines);

    delim = delimiter;
    writerState = WRITER_STATE_NORMAL;

    writeChordsHeader();
    writeChordLines(lines, 1);
    writeChordsFooter();
}
