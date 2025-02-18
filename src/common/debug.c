#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* local includes */
#include "array.h"
#include "common.h"
#include "debug.h"
#include "menu.h"
#include "key_chord.h"
#include "string.h"

static const int MAX_HEADER_WIDTH = 80;
static const int DEBUG_SPACE = 9; /* '[DEBUG] |' == 9 */
static const char* DASHES = "-----------------------------------------------------------------------";

void
debugMsg(bool debug, const char* fmt, ...)
{
    if (!debug) return;

    assert(fmt);

    printf("[DEBUG] ");
    int len = strlen(fmt); /* 1 = '\0' */
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    fputc((fmt[len - 1] == ':' ? ' ' : '\n'), stdout);
}

void
debugMsgWithIndent(int indent, const char* fmt, ...)
{
    assert(fmt);

    printf("[DEBUG] ");
    for (int i = 0; i < indent; i++)
    {
        printf("|    ");
    }

    size_t len = strlen(fmt);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    if (fmt[len - 1] != ' ')
    {
        fputc('\n', stdout);
    }
}

void
debugPrintHeader(const char* header)
{
    assert(header);

    int headerLen = strlen(header); /* add a space to each side */
    if (headerLen > (MAX_HEADER_WIDTH - DEBUG_SPACE))
    {
        debugMsg(true, "| %s", header);
        return;
    }
    else if (headerLen == 0)
    {
        debugMsg(true, "|%s\n", DASHES);
        return;
    }

    int dashCount = (MAX_HEADER_WIDTH - DEBUG_SPACE - headerLen);
    int leftDashes = dashCount / 2;
    int rightDashes = dashCount - leftDashes;
    debugMsg(true, "|%.*s%s%.*s", leftDashes, DASHES, header, rightDashes, DASHES);
}

void
debugPrintHeaderWithIndent(int indent, const char* header)
{
    assert(header);

    int headerLen = strlen(header); /* add a space to each side */
    if (headerLen > (MAX_HEADER_WIDTH - DEBUG_SPACE))
    {
        debugMsgWithIndent(indent, "| %s", header);
        return;
    }
    else if (headerLen == 0)
    {
        debugMsgWithIndent(indent, "|%s\n", DASHES);
        return;
    }

    int dashCount = (MAX_HEADER_WIDTH - DEBUG_SPACE - headerLen);
    int leftDashes = dashCount / 2;
    int rightDashes = dashCount - leftDashes;
    debugMsgWithIndent(indent, "|%.*s%s%.*s", leftDashes, DASHES, header, rightDashes, DASHES);
}

void
debugTextWithLineNumber(const char* text)
{
    if (!text) return;

    const char* current = text;
    size_t i = 1;

    while (*current != '\0')
    {
        printf("[DEBUG] | %4zu | ", i++);
        while (*current != '\n' && *current != '\0')
        {
            printf("%c", *current++);
        }
        if (*current == '\n') printf("%c", *current++);
    }
}

void
debugTextLenWithLineNumber(const char* text, size_t len)
{
    if (!text) return;

    const char* current = text;
    size_t i = 1;

    while (current < text + len)
    {
        printf("[DEBUG] | %4zu | ", i++);
        while (*current != '\n' && current < text + len)
        {
            printf("%c", *current++);
        }
        if (*current == '\n' && current < text + len) printf("%c", *current++);
    }
    if (*(current - 1) != '\n') printf("\n");
}

void
disassembleGrid(
    uint32_t startx, uint32_t starty, uint32_t rows, uint32_t cols, uint32_t wpadding,
    uint32_t hpadding, uint32_t cellw, uint32_t cellh, uint32_t count)
{
    debugPrintHeader(" Grid ");
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| Start X:           %04u", startx, 0);
    debugMsgWithIndent(0, "| Start Y:           %04u", starty);
    debugMsgWithIndent(0, "| Rows:              %04u", rows);
    debugMsgWithIndent(0, "| Columns:           %04u", cols);
    debugMsgWithIndent(0, "| Width padding:     %04u", wpadding);
    debugMsgWithIndent(0, "| Height padding:    %04u", hpadding);
    debugMsgWithIndent(0, "| Cell width:        %04u", cellw);
    debugMsgWithIndent(0, "| Cell height:       %04u", cellh);
    debugMsgWithIndent(0, "| Count:             %04u", count);
    debugMsgWithIndent(0, "|");
    debugPrintHeader("");
}

static void
disassembleHexColor(const MenuHexColor* color)
{
    assert(color);

    debugMsg(true, "| ");
    debugMsgWithIndent(0, "| Hex string:        '%s'",  color->hex);
    debugMsgWithIndent(0, "| Red value:         %#02X", color->r * 255);
    debugMsgWithIndent(0, "| Green value:       %#02X", color->g * 255);
    debugMsgWithIndent(0, "| Blue value:        %#02X", color->b * 255);
    debugMsgWithIndent(0, "| Alpha value:       %#02X", color->a * 255);
    debugMsg(true, "| ");
}

void
disassembleHexColors(const MenuHexColor* colors)
{
    assert(colors);

    for (int i = 0; i < MENU_COLOR_LAST; i++)
    {
        /* TODO refactor */
        switch (i)
        {
        case MENU_COLOR_KEY:
        {
            debugMsg(true, "|----- Foreground Key Color -----");
            break;
        }
        case MENU_COLOR_DELIMITER:
        {
            debugMsg(true, "|---- Foreground Delim Color ----");
            break;
        }
        case MENU_COLOR_PREFIX:
        {
            debugMsg(true, "|--- Foreground Prefix Color ----");
            break;
        }
        case MENU_COLOR_CHORD:
        {
            debugMsg(true, "|--- Foreground Chord Color -----");
            break;
        }
        case MENU_COLOR_BACKGROUND:
        {
            debugMsg(true, "|------- Background color -------");
            break;
        }
        case MENU_COLOR_BORDER:
        {
            debugMsg(true, "|--------- Border color ---------");
            break;
        }
        default: errorMsg("| Got unexpected color index: '%d'.", i); return;
        }

        disassembleHexColor(&colors[i]);
    }
    debugMsgWithIndent(0, "|--------------------------------");
}

static char
getDelim(int* count, char a, char b)
{
    assert(count);

    return ((*count)-- > 1 ? a : b);
}

static void
disassembleMod(const Modifier mod, int indent)
{
    assert(mod);

    debugMsgWithIndent(indent, "| Mods:              ");

    if (!modifierHasAnyActive(mod))
    {
        printf("NONE\n");
        return;
    }

    int count = modifierCount(mod);
    if (modifierIsActive(mod, MOD_CTRL)) printf("CTRL%c", getDelim(&count, '|', '\n'));
    if (modifierIsActive(mod, MOD_META)) printf("ALT%c", getDelim(&count, '|', '\n'));
    if (modifierIsActive(mod, MOD_HYPER)) printf("HYPER%c", getDelim(&count, '|', '\n'));
    if (modifierIsActive(mod, MOD_SHIFT)) printf("SHIFT%c", getDelim(&count, '|', '\n'));
}

static void
disassembleSpecial(SpecialKey special, int indent)
{
    debugMsgWithIndent(indent, "| Special:           ");
    printf("%s|%d\n", specialKeyGetLiteral(special), special);
}

void
disassembleKey(const Key* key)
{
    assert(key);

    debugPrintHeader(" Key ");
    debugMsgWithIndent(0, "|");
    disassembleKeyWithoutHeader(key, 0);
    debugMsgWithIndent(0, "|");
    debugPrintHeader("");
}

void
disassembleKeyWithoutHeader(const Key* key, int indent)
{
    assert(key);

    disassembleMod(key->mods, indent);
    disassembleSpecial(key->special, indent);
    disassembleString(&key->repr, "Key:", indent);
    debugMsgWithIndent(indent, "| Length:            %04d", stringLength(&key->repr));
}

void
disassembleChordFlag(ChordFlag flag, int indent)
{
    assert(flag);

    debugMsgWithIndent(indent, "| Flags:             ");

    if (!chordFlagHasAnyActive(flag))
    {
        printf("WK_FLAG_DEFAULTS\n");
        return;
    }

    int count = chordFlagCount(flag);
    if (chordFlagIsActive(flag, FLAG_KEEP)) printf("KEEP%c", getDelim(&count, '|', '\n'));
    if (chordFlagIsActive(flag, FLAG_CLOSE)) printf("CLOSE%c", getDelim(&count, '|', '\n'));
    if (chordFlagIsActive(flag, FLAG_INHERIT)) printf("INHERIT%c", getDelim(&count, '|', '\n'));
    if (chordFlagIsActive(flag, FLAG_IGNORE)) printf("IGNORE%c", getDelim(&count, '|', '\n'));
    if (chordFlagIsActive(flag, FLAG_UNHOOK)) printf("UNHOOK%c", getDelim(&count, '|', '\n'));
    if (chordFlagIsActive(flag, FLAG_DEFLAG)) printf("DEFLAG%c", getDelim(&count, '|', '\n'));
    if (chordFlagIsActive(flag, FLAG_NO_BEFORE)) printf("NO_BEFORE%c", getDelim(&count, '|', '\n'));
    if (chordFlagIsActive(flag, FLAG_NO_AFTER)) printf("NO_AFTER%c", getDelim(&count, '|', '\n'));
    if (chordFlagIsActive(flag, FLAG_WRITE)) printf("WRITE%c", getDelim(&count, '|', '\n'));
    if (chordFlagIsActive(flag, FLAG_EXECUTE)) printf("EXECUTE%c", getDelim(&count, '|', '\n'));
    if (chordFlagIsActive(flag, FLAG_SYNC_COMMAND)) printf("SYNC_COMMAND%c", getDelim(&count, '|', '\n'));
    if (chordFlagIsActive(flag, FLAG_SYNC_BEFORE)) printf("BEFORE_SYNC%c", getDelim(&count, '|', '\n'));
    if (chordFlagIsActive(flag, FLAG_SYNC_AFTER)) printf("AFTER_SYNC%c", getDelim(&count, '|', '\n'));
}

void
disassembleKeyChord(const KeyChord* keyChord, int indent)
{
    assert(keyChord);

    disassembleKeyWithoutHeader(&keyChord->key, indent);
    debugMsgWithIndent(indent, "| Description:       \"%s\"", keyChord->description);
    debugMsgWithIndent(indent, "| Command:           %{{ %s }}", keyChord->command);
    debugMsgWithIndent(indent, "| Before:            %{{ %s }}", keyChord->before);
    debugMsgWithIndent(indent, "| After:             %{{ %s }}", keyChord->after);
    disassembleChordFlag(keyChord->flags, indent);
}

void
disassembleKeyChordArray(const Array* keyChords, int indent)
{
    assert(keyChords);

    if (indent == 0)
    {
        debugPrintHeaderWithIndent(indent, " KeyChords ");
    }

    ArrayIterator iter = arrayIteratorMake(keyChords);
    const KeyChord* keyChord = NULL;
    while ((keyChord = ARRAY_ITER_NEXT(&iter, const KeyChord)) != NULL)
    {
        debugMsgWithIndent(indent, "|");
        debugMsgWithIndent(indent, "| Chord Index:       %04u", iter.index);
        disassembleKeyChord(keyChord, indent);
        debugMsgWithIndent(indent, "|");
        if (!arrayIsEmpty(&keyChord->keyChords))
        {
            debugMsgWithIndent(
                indent,
                "|------------ Nested KeyChords: %04u -------------",
                arrayLength(&keyChord->keyChords)
            );
            disassembleKeyChordArray(&keyChord->keyChords, indent + 1);
        }
        debugPrintHeaderWithIndent(indent, "-");
    }
    if (indent == 0) printf("\n");
}

void
disassembleKeyChordArrayShallow(const Array* keyChords)
{
    assert(keyChords);

    ArrayIterator iter = arrayIteratorMake(keyChords);
    const KeyChord* keyChord = NULL;
    while ((keyChord = ARRAY_ITER_NEXT(&iter, const KeyChord)) != NULL)
    {
        disassembleKeyChordWithHeader(keyChord, 0);
    }
}

void
disassembleKeyChordWithHeader(const KeyChord* keyChord, int indent)
{
    assert(keyChord);

    debugPrintHeaderWithIndent(indent, " KeyChord ");
    debugMsgWithIndent(indent, "|");
    disassembleKeyChord(keyChord, indent);
    debugMsgWithIndent(indent, "|");
    debugPrintHeaderWithIndent(indent, "");
}

void
disassembleMenu(const Menu* menu)
{
    assert(menu);

    debugPrintHeader(" Menu ");
    debugMsg(true, "|");
    debugMsgWithIndent(0, "| Delimiter:         '%s'",  menu->delimiter);
    debugMsgWithIndent(0, "| Max columns:       %04u",  menu->maxCols);
    debugMsgWithIndent(0, "| Menu width:        %04i",  menu->menuWidth);
    debugMsgWithIndent(0, "| Menu gap:          %04i",  menu->menuGap);
    debugMsgWithIndent(0, "| Width padding:     %04u",  menu->wpadding);
    debugMsgWithIndent(0, "| Height padding:    %04u",  menu->hpadding);
    debugMsgWithIndent(0, "| Cell height:       %04u",  menu->cellHeight);
    debugMsgWithIndent(0, "| Rows:              %04u",  menu->rows);
    debugMsgWithIndent(0, "| Cols:              %04u",  menu->cols);
    debugMsgWithIndent(0, "| Width:             %04u",  menu->width);
    debugMsgWithIndent(0, "| Height:            %04u",  menu->height);
    debugMsgWithIndent(0, "| Window position:   %s",
        (menu->position == MENU_POS_BOTTOM ? "BOTTOM" : "TOP")
    );
    debugMsgWithIndent(0, "| Border width:      %04u",  menu->borderWidth);
    debugMsgWithIndent(0, "|");
    disassembleHexColors(menu->colors);
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| Shell:             '%s'",  menu->shell);
    debugMsgWithIndent(0, "| Font:              '%s'",  menu->font);
    debugMsgWithIndent(0, "| Chords:            %p",    menu->keyChords);
    debugMsgWithIndent(0, "| Chord count:       %04u",  arrayLength(menu->keyChords));
    debugMsgWithIndent(0, "| Debug:             %s",    "true");
    debugMsgWithIndent(0, "| Keys:              %s",    menu->client.keys);
    debugMsgWithIndent(0, "| Transpile:         %s",    menu->client.transpile);
    debugMsgWithIndent(0, "| wks file:          '%s'",  menu->client.wksFile);
    debugMsgWithIndent(0, "| Try script:        %s",    (menu->client.tryScript ? "true" : "false"));
    if (menu->client.script)
    {
        debugMsgWithIndent(0, "| Script:");
        debugMsgWithIndent(0, "|");
        debugTextWithLineNumber(menu->client.script);
        debugMsgWithIndent(0, "|");
    }
    else
    {
        debugMsgWithIndent(0, "| Script:            (null)");
    }
    debugMsgWithIndent(0, "|");
    debugPrintHeader("");
}

void
disassembleStatus(MenuStatus status)
{
    switch (status)
    {
    case MENU_STATUS_RUNNING: debugMsg(true, "WK_STATUS_RUNNING"); break;
    case MENU_STATUS_DAMAGED: debugMsg(true, "WK_STATUS_DAMAGED"); break;
    case MENU_STATUS_EXIT_OK: debugMsg(true, "WK_STATUS_EXIT_OK"); break;
    case MENU_STATUS_EXIT_SOFTWARE: debugMsg(true, "WK_STATUS_EXIT_SOFTWARE"); break;
    default: debugMsg(true, "WK_STATUS_UNKNOWN"); break;
    }
}

void
disassembleString(const String* string, const char* title, int indent)
{
    assert(string);

    if (title == NULL) title = "(null)";

    char buffer[stringLength(string) + 1];
    stringWriteToBuffer(string, buffer);
    debugMsgWithIndent(indent, "| %-19s'%s'", title, buffer);
}
