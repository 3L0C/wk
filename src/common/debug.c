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
#include "key_chord.h"
#include "menu.h"
#include "string.h"

static const int   MAX_HEADER_WIDTH = 80;
static const int   DEBUG_SPACE      = 9; /* '[DEBUG] |' == 9 */
static const char* DASHES           = "-----------------------------------------------------------------------";

void
debugMsg(bool debug, const char* fmt, ...)
{
    if (!debug) return;

    assert(fmt);

    printf("[DEBUG] ");
    int     len = strlen(fmt); /* 1 = '\0' */
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

    size_t  len = strlen(fmt);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    if (fmt[len - 1] != ' ')
    {
        fputc('\n', stdout);
    }
}

static void
debugStringWithIndent(int indent, const char* title, const String* string)
{
    assert(title), assert(string);

    if (stringIsEmpty(string))
    {
        debugMsgWithIndent(indent, "| %-20s %s", title, "(empty_string)");
    }
    else
    {
        char buffer[string->length + 1];
        stringWriteToBuffer(string, buffer);
        debugMsgWithIndent(indent, "| %-20s '%s'", title, buffer);
    }
}

void
debugPrintHeader(const char* header)
{
    assert(header);

    int headerLen = strlen(header); /* add a space to each side */
    int dashCount = (MAX_HEADER_WIDTH - DEBUG_SPACE - headerLen - 2);
    if (dashCount < 2)
    {
        debugMsg(true, "| %s", header);
    }
    else if (headerLen == 0)
    {
        debugMsg(true, "|%s", DASHES);
    }
    else
    {
        int leftDashes  = dashCount / 2;
        int rightDashes = dashCount - leftDashes;
        debugMsg(true, "|%.*s %s %.*s", leftDashes, DASHES, header, rightDashes, DASHES);
    }
}

void
debugPrintHeaderWithIndent(int indent, const char* header)
{
    assert(header);

    int headerLen = strlen(header); /* add a space to each side */
    int dashCount = (MAX_HEADER_WIDTH - DEBUG_SPACE - headerLen - 2);
    if (dashCount < 2)
    {
        debugMsgWithIndent(indent, "| %s", header);
    }
    else if (headerLen == 0)
    {
        debugMsgWithIndent(indent, "|%s", DASHES);
    }
    else
    {
        int leftDashes  = dashCount / 2;
        int rightDashes = dashCount - leftDashes;
        debugMsgWithIndent(indent, "|%.*s %s %.*s", leftDashes, DASHES, header, rightDashes, DASHES);
    }
}

void
debugTextWithLineNumber(const char* text)
{
    if (!text) return;

    const char* current = text;
    size_t      i       = 1;

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
    size_t      i       = 1;

    while (current < text + len)
    {
        printf("[DEBUG] | %4zu | ", i++);
        while (*current != '\n' && current < text + len)
        {
            if (*current == '\0')
            {
                printf("␀");
            }
            else
            {
                printf("%c", *current);
            }
            current++;
        }
        if (*current == '\n' && current < text + len) printf("%c", *current++);
    }
    if (*(current - 1) != '\n') printf("\n");
}

void
disassembleGrid(
    uint32_t startx,
    uint32_t starty,
    uint32_t rows,
    uint32_t cols,
    uint32_t wpadding,
    uint32_t hpadding,
    uint32_t cellw,
    uint32_t cellh,
    uint32_t count)
{
    debugPrintHeader("Grid");
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| %-20s %04u", "Start X:", startx, 0);
    debugMsgWithIndent(0, "| %-20s %04u", "Start Y:", starty);
    debugMsgWithIndent(0, "| %-20s %04u", "Rows:", rows);
    debugMsgWithIndent(0, "| %-20s %04u", "Columns:", cols);
    debugMsgWithIndent(0, "| %-20s %04u", "Width padding:", wpadding);
    debugMsgWithIndent(0, "| %-20s %04u", "Height padding:", hpadding);
    debugMsgWithIndent(0, "| %-20s %04u", "Cell width:", cellw);
    debugMsgWithIndent(0, "| %-20s %04u", "Cell height:", cellh);
    debugMsgWithIndent(0, "| %-20s %04u", "Count:", count);
    debugMsgWithIndent(0, "|");
    debugPrintHeader("");
}

static void
disassembleHexColor(const MenuHexColor* color)
{
    assert(color);

    debugMsg(true, "|");
    debugMsgWithIndent(0, "| %-20s '%s'", "Hex string:", color->hex);
    debugMsgWithIndent(0, "| %-20s %#02X", "Red value:", color->r * 255);
    debugMsgWithIndent(0, "| %-20s %#02X", "Green value:", color->g * 255);
    debugMsgWithIndent(0, "| %-20s %#02X", "Blue value:", color->b * 255);
    debugMsgWithIndent(0, "| %-20s %#02X", "Alpha value:", color->a * 255);
    debugMsg(true, "|");
}

void
disassembleHexColors(const MenuHexColor* colors)
{
    assert(colors);

    debugMsg(true, "|");
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
        case MENU_COLOR_TITLE:
        {
            debugMsg(true, "|--- Foreground Title Color -----");
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
    debugMsg(true, "|--------------------------------");
    debugMsg(true, "|");
}

static void
disassembleMod(const Modifier mod, int indent)
{
    debugMsgWithIndent(indent, "| %-20s ", "Mods:");

    if (!modifierHasAnyActive(mod))
    {
        printf("NONE\n");
        return;
    }

    int count = modifierCount(mod);
    if (modifierIsActive(mod, MOD_CTRL)) printf("CTRL%s", getSeparator(&count, "|", "\n"));
    if (modifierIsActive(mod, MOD_META)) printf("ALT%s", getSeparator(&count, "|", "\n"));
    if (modifierIsActive(mod, MOD_HYPER)) printf("HYPER%s", getSeparator(&count, "|", "\n"));
    if (modifierIsActive(mod, MOD_SHIFT)) printf("SHIFT%s", getSeparator(&count, "|", "\n"));
}

static void
disassembleSpecial(SpecialKey special, int indent)
{
    debugMsgWithIndent(indent, "| %-20s ", "Special:");
    printf("%s | %d\n", specialKeyGetLiteral(special), special);
}

void
disassembleKey(const Key* key)
{
    assert(key);

    debugMsg(true, "");
    debugPrintHeader("Key");
    debugMsgWithIndent(0, "|");
    disassembleKeyWithoutHeader(key, 0);
    debugMsgWithIndent(0, "|");
    debugPrintHeader("");
    debugMsg(true, "");
}

void
disassembleKeyWithoutHeader(const Key* key, int indent)
{
    assert(key);

    disassembleMod(key->mods, indent);
    disassembleSpecial(key->special, indent);
    disassembleString(&key->repr, "Key:", indent);
    debugMsgWithIndent(indent, "| %-20s %04d", "Length:", stringLength(&key->repr));
}

void
disassembleArrayAsText(const Array* arr, const char* title)
{
    assert(arr), assert(title);

    debugPrintHeader(title);
    debugMsg(true, "| ");
    debugTextLenWithLineNumber(ARRAY_AS(arr, char), arrayLength(arr));
    debugMsg(true, "| ");
    debugPrintHeader("");
}

void
disassembleChordFlag(ChordFlag flag, int indent)
{
    debugMsgWithIndent(indent, "| %-20s ", "Flags");

    if (!chordFlagHasAnyActive(flag))
    {
        printf("WK_FLAG_DEFAULTS\n");
        return;
    }

    int count = chordFlagCount(flag);
    if (chordFlagIsActive(flag, FLAG_KEEP)) printf("KEEP%s", getSeparator(&count, "|", "\n"));
    if (chordFlagIsActive(flag, FLAG_CLOSE)) printf("CLOSE%s", getSeparator(&count, "|", "\n"));
    if (chordFlagIsActive(flag, FLAG_INHERIT)) printf("INHERIT%s", getSeparator(&count, "|", "\n"));
    if (chordFlagIsActive(flag, FLAG_IGNORE)) printf("IGNORE%s", getSeparator(&count, "|", "\n"));
    if (chordFlagIsActive(flag, FLAG_UNHOOK)) printf("UNHOOK%s", getSeparator(&count, "|", "\n"));
    if (chordFlagIsActive(flag, FLAG_DEFLAG)) printf("DEFLAG%s", getSeparator(&count, "|", "\n"));
    if (chordFlagIsActive(flag, FLAG_NO_BEFORE)) printf("NO_BEFORE%s", getSeparator(&count, "|", "\n"));
    if (chordFlagIsActive(flag, FLAG_NO_AFTER)) printf("NO_AFTER%s", getSeparator(&count, "|", "\n"));
    if (chordFlagIsActive(flag, FLAG_WRITE)) printf("WRITE%s", getSeparator(&count, "|", "\n"));
    if (chordFlagIsActive(flag, FLAG_EXECUTE)) printf("EXECUTE%s", getSeparator(&count, "|", "\n"));
    if (chordFlagIsActive(flag, FLAG_SYNC_COMMAND)) printf("SYNC_COMMAND%s", getSeparator(&count, "|", "\n"));
    if (chordFlagIsActive(flag, FLAG_SYNC_BEFORE)) printf("BEFORE_SYNC%s", getSeparator(&count, "|", "\n"));
    if (chordFlagIsActive(flag, FLAG_SYNC_AFTER)) printf("AFTER_SYNC%s", getSeparator(&count, "|", "\n"));
    if (chordFlagIsActive(flag, FLAG_UNWRAP)) printf("UNWRAP%s", getSeparator(&count, "|", "\n"));
}

void
disassembleKeyChord(const KeyChord* keyChord, int indent)
{
    assert(keyChord);

    disassembleKeyWithoutHeader(&keyChord->key, indent);
    debugStringWithIndent(indent, "Description:", &keyChord->description);
    debugStringWithIndent(indent, "Command:", &keyChord->command);
    debugStringWithIndent(indent, "Before:", &keyChord->before);
    debugStringWithIndent(indent, "After:", &keyChord->after);
    disassembleChordFlag(keyChord->flags, indent);
}

void
disassembleKeyChordArray(const Array* keyChords, int indent)
{
    assert(keyChords);

    debugMsg(true, "");
    if (indent == 0) debugPrintHeader("KeyChords");

    forEach(keyChords, const KeyChord, keyChord)
    {
        debugMsgWithIndent(indent, "|");
        debugMsgWithIndent(indent, "| %-20s %04zu", "Chord Index:", iter.index);
        disassembleKeyChord(keyChord, indent);
        debugMsgWithIndent(indent, "|");
        if (!arrayIsEmpty(&keyChord->keyChords))
        {
            debugMsgWithIndent(
                indent,
                "|------------ Nested KeyChords: %04u -------------",
                arrayLength(&keyChord->keyChords));
            disassembleKeyChordArray(&keyChord->keyChords, indent + 1);
        }
        debugPrintHeaderWithIndent(indent, "");
    }
    debugMsg(true, "");
}

void
disassembleKeyChordArrayShallow(const Array* keyChords)
{
    assert(keyChords);

    debugMsg(true, "");
    forEach(keyChords, const KeyChord, keyChord)
    {
        disassembleKeyChordWithHeader(keyChord, 0);
    }
    debugMsg(true, "");
}

void
disassembleKeyChordWithHeader(const KeyChord* keyChord, int indent)
{
    assert(keyChord);

    debugPrintHeaderWithIndent(indent, "KeyChord");
    debugMsgWithIndent(indent, "|");
    disassembleKeyChord(keyChord, indent);
    debugMsgWithIndent(indent, "|");
    debugPrintHeaderWithIndent(indent, "");
}

void
disassembleMenu(const Menu* menu)
{
    assert(menu);

    debugMsg(true, "");
    debugPrintHeader("Menu");
    debugMsg(true, "|");
    debugMsgWithIndent(0, "| %-20s '%s'", "Delimiter:", menu->delimiter);
    debugMsgWithIndent(0, "| %-20s '%s'", "Shell:", menu->shell);
    debugMsgWithIndent(0, "| %-20s '%s'", "Font:", menu->font);
    debugMsgWithIndent(0, "| %-20s '%s'", "Array Keys:", menu->implicitArrayKeys);
    debugMsgWithIndent(0, "| %-20s %04.4f", "Border Radius:", menu->borderRadius);
    disassembleHexColors(menu->colors);
    debugMsgWithIndent(0, "| %-20s %s", "Keys:", menu->client.keys);
    debugMsgWithIndent(0, "| %-20s %s", "Transpile:", menu->client.transpile);
    debugMsgWithIndent(0, "| %-20s '%s'", "wks file:", menu->client.wksFile);
    debugMsgWithIndent(0, "| %-20s %s", "Try script:", (menu->client.tryScript ? "true" : "false"));
    if (!arrayIsEmpty(&menu->client.script))
    {
        debugMsgWithIndent(0, "| Script:");
        debugMsgWithIndent(0, "|");
        debugTextWithLineNumber(ARRAY_AS(&menu->client.script, char));
        debugMsgWithIndent(0, "|");
    }
    else
    {
        debugMsgWithIndent(0, "| %-20s (null)", "Script:");
    }
    debugMsgWithIndent(0, "| %-20s %04u", "Max columns:", menu->maxCols);
    debugMsgWithIndent(0, "| %-20s %04i", "Menu width:", menu->menuWidth);
    debugMsgWithIndent(0, "| %-20s %04i", "Menu gap:", menu->menuGap);
    debugMsgWithIndent(0, "| %-20s %04u", "Width padding:", menu->wpadding);
    debugMsgWithIndent(0, "| %-20s %04u", "Height padding:", menu->hpadding);
    debugMsgWithIndent(0, "| %-20s %04u", "Cell height:", menu->cellHeight);
    debugMsgWithIndent(0, "| %-20s %04u", "Title height:", menu->titleHeight);
    debugMsgWithIndent(0, "| %-20s %04u", "Rows:", menu->rows);
    debugMsgWithIndent(0, "| %-20s %04u", "Cols:", menu->cols);
    debugMsgWithIndent(0, "| %-20s %04u", "Width:", menu->width);
    debugMsgWithIndent(0, "| %-20s %04u", "Height:", menu->height);
    debugMsgWithIndent(0, "| %-20s %04u", "Border width:", menu->borderWidth);
    debugMsgWithIndent(0, "| %-20s %04u", "Delay:", menu->delay);
    debugMsgWithIndent(0, "| %-20s %04u", "Keep Delay:", menu->keepDelay);
    debugMsgWithIndent(0, "| %-20s %s", "Wrap Cmd:", menu->wrapCmd);
    debugStringWithIndent(0, "Wrap Cmd", &menu->wrapCmd);
    debugMsgWithIndent(
        0,
        "| %-20s %s",
        "Window position:",
        (menu->position == MENU_POS_BOTTOM ? "BOTTOM" : "TOP"));
    debugMsgWithIndent(0, "| %-20s %s", "Debug:", "true");
    debugMsgWithIndent(0, "| %-20s %s", "Sort:", menu->sort ? "true" : "false");
    debugMsgWithIndent(0, "| %-20s %s", "Dirty:", menu->dirty ? "true" : "false");
    debugMsgWithIndent(0, "|");
    debugPrintHeader("");
    debugMsg(true, "");
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

    char buffer[string->length + 1];
    stringWriteToBuffer(string, buffer);
    debugMsgWithIndent(indent, "| %-20s '%s'", title, buffer);
}
