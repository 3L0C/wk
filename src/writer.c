#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>

#include "lib/types.h"

#include "line.h"
#include "scanner.h"
#include "token.h"
#include "writer.h"

static const char* delim;

static void writeChordLines(LineArray* lines, int indent);

static void
writeChordsHeader(void)
{
    printf("#ifndef WK_CHORDS_H_\n");
    printf("#define WK_CHORDS_H_\n");
    printf("\n");
    printf("#include \"lib/common.h\"\n");
    printf("#include \"lib/types.h\"\n");
    printf("\n");
    printf("/*                    mods,        special,         key,  desc, hint, command,  before, after, keep,  unhook, nobefore, noafter, write, chords */\n");
    printf("#define NULL_CHORD  { WK_MOD_NONE, WK_SPECIAL_NONE, NULL, NULL, NULL, NULL,     NULL,   NULL,  false, false,  false,    false,   false, NULL }\n");
    printf("#define PREFIX(...) (Chord[]){ __VA_ARGS__, NULL_CHORD }\n");
    printf("#define CHORDS(...) { __VA_ARGS__, NULL_CHORD }\n");
    printf("\n");
    printf("/* mods,    specials,  key,    description,   hint, */\n");
    printf("/* command, clen */\n");
    printf("/* before, blen */\n");
    printf("/* after, alen */\n");
    printf("/* keep,    unhook,    nobefore,    noafter,    write,    chords */\n");
    printf("const Chord chords[] = CHORDS(\n");
}

static void
writeChordsFooter(void)
{
    printf(");\n");
    printf("\n");
    printf("#endif /* WK_CHORDS_H_ */\n");
}

static int
countMods(int mods)
{
    int result = 0;
    if (!IS_MOD(mods)) return result;
    if (IS_CTRL(mods)) result++;
    if (IS_ALT(mods)) result++;
    if (IS_HYPER(mods)) result++;
    if (IS_SHIFT(mods)) result++;
    return result;
}

static void
writeChordMods(int mods)
{
    int count = countMods(mods);
    if (!IS_MOD(mods)) printf("WK_MOD_NONE");
    if (IS_CTRL(mods)) printf("WK_MOD_CTRL%s", count-- > 1 ? "|" : "");
    if (IS_ALT(mods)) printf("WK_MOD_ALT%s", count-- > 1 ? "|" : "");
    if (IS_HYPER(mods)) printf("WK_MOD_HYPER%s", count-- > 1 ? "|" : "");
    if (IS_SHIFT(mods)) printf("WK_MOD_SHIFT%s", count-- > 1 ? "|" : "");
    printf(", ");
}

static void
writeChordSpecial(Token* token)
{
    switch (token->type)
    {
    /* is special */
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
    /* not special */
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
rstrip(Token* token, bool flag)
{
    size_t index = token->length - 1;
    if (!flag) return index + 1;
    const char* lexeme = token->start;
    while (index && isspace(lexeme[index])) index--;
    return index + 1;
}

static void
writeChordEscString(Token* token, bool flag)
{
    size_t length = rstrip(token, flag);
    for (size_t i = 0; i < length; i++)
    {
        char c = token->start[i];
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
writeChordInterp(Token* token, Line* line)
{
    switch (token->type)
    {
    case TOKEN_INDEX:       printf("%d", line->index); break;
    case TOKEN_INDEX_ONE:   printf("%d", line->index + 1); break;
    case TOKEN_THIS_KEY:    writeChordEscKey(&line->key); break;
    case TOKEN_COMM_INTERP: /* FALLTHROUGH */
    case TOKEN_DESC_INTERP: writeChordEscString(token, false); break;
    default: writeChordEscString(token, true); break;
    }

}

static void
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
writeChordModStr(int mods)
{
    if (!IS_MOD(mods)) return;
    if (IS_CTRL(mods)) printf("C-");
    if (IS_ALT(mods)) printf("A-");
    if (IS_HYPER(mods)) printf("H-");
    if (IS_SHIFT(mods)) printf("S-");
}

static void
writeChordHint(Line* line)
{
    printf("\"");
    writeChordModStr(line->mods);
    writeChordEscKey(&line->key);
    printf(" %s ", delim);
    writeChordRawString(&line->description, line);
    printf("\",\n");
}

static void
writeChordBool(bool flag)
{
    printf("%s, ", flag ? "true" : "false");
}

static void
writeChordLine(Line* line, int indent)
{
    printf("%*s", indent * 4, " ");
    writeChordMods(line->mods);
    writeChordSpecial(&line->key);
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
    writeChordBool(line->keep);
    writeChordBool(line->unhook);
    writeChordBool(line->nobefore);
    writeChordBool(line->noafter);
    writeChordBool(line->write);

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
        printf("%*s", indent * 4, " "); /* print indentation */
        printf("{\n");
        writeChordLine(&lines->lines[i], indent + 1);
        printf("%*s}", indent * 4, " "); /* print indentation */
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

    writeChordsHeader();
    writeChordLines(lines, 1);
    writeChordsFooter();
}
