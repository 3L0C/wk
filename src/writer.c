#include <stdio.h>

#include "line.h"
#include "scanner.h"
#include "token.h"
#include "writer.h"

static void writeChordLines(LineArray* lines, int indent);

static void
writeChordsHeader(void)
{
    printf("#ifndef WK_CHORDS_H_\n");
    printf("#define WK_CHORDS_H_\n");
    printf("\n");
    printf("#include \"lib/common.h\"");
    printf("\n");
    printf("#define NULL_CHORD  { WK_MOD_NONE, WK_SPECIAL_NONE, NULL, NULL, NULL, NULL, NULL, false, false, false, false, false, NULL }\n");
    printf("#define PREFIX(...) (const Chord[]){ __VA_ARGS__, { NULL_CHORD } }\n");
    printf("#define CHORDS(...) { __VA_ARGS__, { NULL_CHORD } }\n");
    printf("\n");
    printf("const Chord chords[] = CHORDS(\n");
    printf("    /* mods,    specials,    key,    description,    command,    before,    after, */\n");
    printf("    /* keep,    unhook,    nobefore,    noafter,    write,    chords */\n");
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
    if (mods == WK_MOD_NONE) return result;
    if ((mods & WK_MOD_CTRL) == WK_MOD_CTRL) result++;
    if ((mods & WK_MOD_ALT) == WK_MOD_ALT) result++;
    if ((mods & WK_MOD_HYPER) == WK_MOD_HYPER) result++;
    if ((mods & WK_MOD_SHIFT) == WK_MOD_SHIFT) result++;
    return result;
}

static void
writeChordMods(int mods)
{
    int count = countMods(mods);
    if (mods == WK_MOD_NONE) printf("WK_MOD_NONE");
    if ((mods & WK_MOD_CTRL) == WK_MOD_CTRL) printf("WK_MOD_CTRL%s", count-- > 1 ? "|" : "");
    if ((mods & WK_MOD_ALT) == WK_MOD_ALT) printf("WK_MOD_ALT%s", count-- > 1 ? "|" : "");
    if ((mods & WK_MOD_HYPER) == WK_MOD_HYPER) printf("WK_MOD_HYPER%s", count-- > 1 ? "|" : "");
    if ((mods & WK_MOD_SHIFT) == WK_MOD_SHIFT) printf("WK_MOD_SHIFT%s", count-- > 1 ? "|" : "");
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

static void
writeChordEscString(Token* token)
{
    for (size_t i = 0; i < token->length; i++)
    {
        char c = token->start[i];
        switch (c)
        {
        case '\\': printf("\\"); break;
        case '\"': printf("\\\""); break;
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
    default: writeChordEscString(token); break;
    }

}

static void
writeChordString(TokenArray* array, Line* line)
{

    printf("\"");
    for (size_t i = 0; i < array->count; i++)
    {
        writeChordInterp(&array->tokens[i], line);
    }
    printf("\", ");
}

static void
writeChordBool(bool flag)
{
    printf("%s, ", flag ? "true" : "false");
}

static void
writeChordLine(Line* line, int indent)
{
    writeChordMods(line->mods);
    writeChordSpecial(&line->key);
    writeChordKey(&line->key);
    writeChordString(&line->description, line);
    writeChordString(&line->command, line);
    writeChordString(&line->before, line);
    writeChordString(&line->after, line);
    printf("\n  %*s", indent * 4, " ");
    writeChordBool(line->keep);
    writeChordBool(line->unhook);
    writeChordBool(line->nobefore);
    writeChordBool(line->noafter);
    writeChordBool(line->write);
    if (line->array.count != 0)
    {
        printf("PREFIX(\n");
        writeChordLines(&line->array, indent + 1);
        printf("%*s)", indent * 4, " ");
    }
    else
    {
        printf("NULL ");
    }
}

static void
writeChordLines(LineArray* lines, int indent)
{
    for (size_t i = 0; i < lines->count; i++)
    {
        printf("%*s", indent * 4, " "); /* print indentation */
        printf("{ ");
        writeChordLine(&lines->lines[i], indent);
        printf("}");
        if (i + 1 != lines->count)
        {
            printf(",");
        }
        printf("\n");
    }
}

void
writeChords(LineArray* lines)
{
    writeChordsHeader();
    writeChordLines(lines, 1);
    writeChordsFooter();
}
