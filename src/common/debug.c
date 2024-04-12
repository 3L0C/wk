#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "debug.h"
#include "menu.h"
#include "types.h"
#include "util.h"

static const int MAX_HEADER_WIDTH = 52;
static const int DEBUG_SPACE = 9; /* '[DEBUG] |' == 9 */
static const char* DASHES = "-------------------------------------------";

static char
getDelim(int* count, char a, char b)
{
    assert(count);

    return ((*count)-- > 1 ? a : b);
}

static void
printDebug(int indent)
{
    printf("[DEBUG] %*s", indent * 8, (indent ? " " : ""));
}

static void
debugMod(const WkMods* mods, int indent)
{
    assert(mods);

    debugMsgWithIndent(indent, "| Mods:              ");

    if (!IS_MOD(*mods))
    {
        printf("NONE\n");
        return;
    }

    printf("| Mods                ");
    int count = COUNT_MODS(*mods);
    if (mods->ctrl) printf("CTRL%c", getDelim(&count, '|', '\n'));
    if (mods->alt) printf("ALT%c", getDelim(&count, '|', '\n'));
    if (mods->hyper) printf("HYPER%c", getDelim(&count, '|', '\n'));
    if (mods->shift) printf("SHIFT%c", getDelim(&count, '|', '\n'));
}

static bool
debugSpecial(WkSpecial special, int indent)
{
    debugMsgWithIndent(indent, "| Special:           ");
    const char* text = NULL;
    bool flag = true;

    switch (special)
    {
    case WK_SPECIAL_NONE:       text = "WK_SPECIAL_NONE";       flag = false; break;
    case WK_SPECIAL_LEFT:       text = "WK_SPECIAL_LEFT";       break;
    case WK_SPECIAL_RIGHT:      text = "WK_SPECIAL_RIGHT";      break;
    case WK_SPECIAL_UP:         text = "WK_SPECIAL_UP";         break;
    case WK_SPECIAL_DOWN:       text = "WK_SPECIAL_DOWN";       break;
    case WK_SPECIAL_TAB:        text = "WK_SPECIAL_TAB";        break;
    case WK_SPECIAL_SPACE:      text = "WK_SPECIAL_SPACE";      break;
    case WK_SPECIAL_RETURN:     text = "WK_SPECIAL_RETURN";     break;
    case WK_SPECIAL_DELETE:     text = "WK_SPECIAL_DELETE";     break;
    case WK_SPECIAL_ESCAPE:     text = "WK_SPECIAL_ESCAPE";     break;
    case WK_SPECIAL_HOME:       text = "WK_SPECIAL_HOME";       break;
    case WK_SPECIAL_PAGE_UP:    text = "WK_SPECIAL_PAGE_UP";    break;
    case WK_SPECIAL_PAGE_DOWN:  text = "WK_SPECIAL_PAGE_DOWN";  break;
    case WK_SPECIAL_END:        text = "WK_SPECIAL_END";        break;
    case WK_SPECIAL_BEGIN:      text = "WK_SPECIAL_BEGIN";      break;
    default: text = "UNKNOWN"; flag = false; break;
    }

    printf("%s|%d\n", text, special);
    return flag;
}

static bool
debugSpecialRepr(WkSpecial special, int indent)
{
    debugMsgWithIndent(indent, "| Key:               ");
    const char* text = NULL;
    bool flag = true;

    switch (special)
    {
    case WK_SPECIAL_NONE:       text = "NONE";          flag = false; break;
    case WK_SPECIAL_LEFT:       text = "'Left'";        break;
    case WK_SPECIAL_RIGHT:      text = "'Right'";       break;
    case WK_SPECIAL_UP:         text = "'Up'";          break;
    case WK_SPECIAL_DOWN:       text = "'Down'";        break;
    case WK_SPECIAL_TAB:        text = "'TAB'";         break;
    case WK_SPECIAL_SPACE:      text = "'SPC'";         break;
    case WK_SPECIAL_RETURN:     text = "'RET'";         break;
    case WK_SPECIAL_DELETE:     text = "'DEL'";         break;
    case WK_SPECIAL_ESCAPE:     text = "'ESC'";         break;
    case WK_SPECIAL_HOME:       text = "'Home'";        break;
    case WK_SPECIAL_PAGE_UP:    text = "'PgUp'";        break;
    case WK_SPECIAL_PAGE_DOWN:  text = "'PgDown'";      break;
    case WK_SPECIAL_END:        text = "'End'";         break;
    case WK_SPECIAL_BEGIN:      text = "'Begin'";       break;
    default: text = "UNKNOWN"; flag = false; break;
    }

    printf("%s\n", text);
    return flag;
}

static void
debugString(const char* text, const char* value, int indent)
{
    assert(text);

    printDebug(indent);
    printf("| %-19s '%s'\n", text, value);
}

static void
debugFlags(const WkFlags* flags, int indent)
{
    assert(flags);

    debugMsgWithIndent(indent, "| Flags:             ");

    if (!HAS_FLAG(*flags))
    {
        printf("WK_FLAG_DEFAULTS\n");
        return;
    }

    int count = COUNT_FLAGS(*flags);
    if (flags->keep) printf("KEEP%c", getDelim(&count, '|', '\n'));
    if (flags->close) printf("CLOSE%c", getDelim(&count, '|', '\n'));
    if (flags->inherit) printf("INHERIT%c", getDelim(&count, '|', '\n'));
    if (flags->ignore) printf("IGNORE%c", getDelim(&count, '|', '\n'));
    if (flags->unhook) printf("UNHOOK%c", getDelim(&count, '|', '\n'));
    if (flags->deflag) printf("DEFLAG%c", getDelim(&count, '|', '\n'));
    if (flags->nobefore) printf("NO_BEFORE%c", getDelim(&count, '|', '\n'));
    if (flags->noafter) printf("NO_AFTER%c", getDelim(&count, '|', '\n'));
    if (flags->write) printf("WRITE%c", getDelim(&count, '|', '\n'));
    if (flags->syncCommand) printf("SYNC_COMMAND%c", getDelim(&count, '|', '\n'));
    if (flags->beforeSync) printf("BEFORE_SYNC%c", getDelim(&count, '|', '\n'));
    if (flags->afterSync) printf("AFTER_SYNC%c", getDelim(&count, '|', '\n'));
}

void
disassembleKeyChord(const WkKeyChord* keyChord, int indent)
{
    assert(keyChord);

    debugMod(&keyChord->mods, indent);
    debugSpecial(keyChord->special, indent);
    debugMsgWithIndent(indent, "| Key:               '%s'", keyChord->key);
    debugMsgWithIndent(indent, "| Description:       \"%s\"", keyChord->description);
    debugMsgWithIndent(indent, "| Hint:              '%s'", keyChord->hint);
    debugMsgWithIndent(indent, "| Command:           %{{%s}}", keyChord->command);
    debugMsgWithIndent(indent, "| Before:            %{{%s}}", keyChord->before);
    debugMsgWithIndent(indent, "| After:             %{{%s}}", keyChord->after);
    debugFlags(&keyChord->flags, indent);
}

void
debugKeyChord(const WkKeyChord* keyChord, int indent)
{
    assert(keyChord);

    debugPrintHeaderWithIndent(indent, "KeyChord");
    debugMsgWithIndent(indent, "|");
    disassembleKeyChord(keyChord, indent);
    debugMsgWithIndent(indent, "|");
    debugPrintHeaderWithIndent(indent, "");
    printf("\n");
}

void
debugKeyChords(const WkKeyChord* keyChords, int indent)
{
    assert(keyChords);

    if (indent == 0)
    {
        debugPrintHeaderWithIndent(indent, "KeyChords");
    }
    for (uint32_t i = 0; keyChords[i].key; i++)
    {
        debugMsgWithIndent(indent, "|");
        debugMsgWithIndent(indent, "| Chord Index:       %04u", i);
        disassembleKeyChord(&keyChords[i], indent);
        debugMsgWithIndent(indent, "|");
        if (keyChords[i].keyChords)
        {
            debugMsgWithIndent(
                indent,
                "|------------ Nested KeyChords: %04u -------------",
                countKeyChords(keyChords[i].keyChords)
            );
            debugKeyChords(keyChords[i].keyChords, indent + 1);
        }
        debugPrintHeaderWithIndent(indent, "");
    }
    if (indent == 0) printf("\n");
}

void
debugKeyChordsShallow(const WkKeyChord* keyChords, uint32_t len)
{
    assert(keyChords);

    for (uint32_t i = 0; i < len; i++)
    {
        debugKeyChord(&keyChords[i], 0);
    }
}

void
debugGrid(
    uint32_t startx, uint32_t starty, uint32_t rows, uint32_t cols, uint32_t wpadding,
    uint32_t hpadding, uint32_t cellw, uint32_t cellh, uint32_t count
)
{
    debugPrintHeader("Grid");
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
    printf("\n");
}

static void
debugHexColor(const WkHexColor* color)
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
debugHexColors(const WkHexColor* colors)
{
    assert(colors);

    for (int i = 0; i < WK_COLOR_LAST; i++)
    {
        printDebug(0);
        switch (i)
        {
        case WK_COLOR_FOREGROUND: printf("|------- Foreground color -------\n"); break;
        case WK_COLOR_BACKGROUND: printf("|------- Background color -------\n"); break;
        case WK_COLOR_BORDER:     printf("|--------- Border color ---------\n"); break;
        default: errorMsg("| Got unexpected color index: '%d'.", i); return;
        }

        debugHexColor(&colors[i]);
    }
    printDebug(0);
    printf("|--------------------------------\n");
}

void
debugKey(const WkKey* key)
{
    assert(key);

    debugPrintHeader("Key");
    debugMsgWithIndent(0, "|");
    debugMod(&key->mods, 0);
    if (debugSpecial(key->special, 0))
    {
        debugSpecialRepr(key->special, 0);
    }
    else
    {
        debugString("Key", key->key, 0);
    }
    debugMsgWithIndent(0, "| Length:            %04d", key->len);
    debugMsgWithIndent(0, "|");
    debugPrintHeader("");
    printf("\n");
}

void
debugMenu(const WkMenu* menu)
{
    assert(menu);

    debugPrintHeader("Menu");
    debugMsg(true, "|");
    debugMsgWithIndent(0, "| Delimiter:         '%s'",  menu->delimiter);
    debugMsgWithIndent(0, "| Max columns:       %04u",  menu->maxCols);
    debugMsgWithIndent(0, "| Window width:      %04i",  menu->windowWidth);
    debugMsgWithIndent(0, "| Window gap:        %04i",  menu->windowGap);
    debugMsgWithIndent(0, "| Width padding:     %04u",  menu->wpadding);
    debugMsgWithIndent(0, "| Height padding:    %04u",  menu->hpadding);
    debugMsgWithIndent(0, "| Cell height:       %04u",  menu->cellHeight);
    debugMsgWithIndent(0, "| Rows:              %04u",  menu->rows);
    debugMsgWithIndent(0, "| Cols:              %04u",  menu->cols);
    debugMsgWithIndent(0, "| Width:             %04u",  menu->width);
    debugMsgWithIndent(0, "| Height:            %04u",  menu->height);
    debugMsgWithIndent(0, "| Window position:   %s",
        (menu->position == WK_WIN_POS_BOTTOM ? "BOTTOM" : "TOP")
    );
    debugMsgWithIndent(0, "| Border width:      %04u",  menu->borderWidth);
    debugMsgWithIndent(0, "|");
    debugHexColors(menu->colors);
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| Shell:             '%s'",  menu->shell);
    debugMsgWithIndent(0, "| Font:              '%s'",  menu->font);
    debugMsgWithIndent(0, "| Chords:            %p",    menu->keyChords);
    debugMsgWithIndent(0, "| Chord count:       %04u",  menu->keyChordCount);
    debugMsgWithIndent(0, "| Debug:             %s",    "true");
    debugMsgWithIndent(0, "| Keys:              %s",    menu->client.keys);
    debugMsgWithIndent(0, "| Transpile:         %s",    menu->client.transpile);
    debugMsgWithIndent(0, "| Chords file:       '%s'",  menu->client.keyChordsFile);
    debugMsgWithIndent(0, "| Try script:        %s",    (menu->client.tryScript ? "true" : "false"));
    debugMsgWithIndent(0, "| Script:            %s",    menu->client.script.string);
    debugMsgWithIndent(0, "| Script capacity:   %04zu", menu->client.script.capacity);
    debugMsgWithIndent(0, "| Script count:      %04zu", menu->client.script.count);
    debugMsgWithIndent(0, "|");
    debugPrintHeader("");
    printf("\n");
}

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

    fputc((fmt[len] == ':' ? ' ' : '\n'), stdout);
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

    static const int HEADER_SPACE = 2;
    int headerLen = strlen(header) + HEADER_SPACE; /* add a space to each side */
    if (headerLen > (MAX_HEADER_WIDTH - DEBUG_SPACE))
    {
        debugMsg(true, "| %s", header);
        return;
    }
    else if (headerLen - HEADER_SPACE == 0)
    {
        debugMsg(true, "|%s", DASHES);
        return;
    }

    int dashCount = (MAX_HEADER_WIDTH - DEBUG_SPACE - headerLen);
    int leftDashes = dashCount / 2;
    int rightDashes = dashCount - leftDashes;
    debugMsg(true, "|%.*s %s %.*s", leftDashes, DASHES, header, rightDashes, DASHES);
}

void
debugPrintHeaderWithIndent(int indent, const char* header)
{
    assert(header);

    static const int HEADER_SPACE = 2;
    int headerLen = strlen(header) + HEADER_SPACE; /* add a space to each side */
    if (headerLen > (MAX_HEADER_WIDTH - DEBUG_SPACE))
    {
        debugMsgWithIndent(indent, "| %s", header);
        return;
    }
    else if (headerLen - HEADER_SPACE == 0)
    {
        debugMsgWithIndent(indent, "|%s", DASHES);
        return;
    }

    int dashCount = (MAX_HEADER_WIDTH - DEBUG_SPACE - headerLen);
    int leftDashes = dashCount / 2;
    int rightDashes = dashCount - leftDashes;
    debugMsgWithIndent(indent, "|%.*s %s %.*s", leftDashes, DASHES, header, rightDashes, DASHES);
}

void
debugStatus(WkStatus status)
{
    printDebug(0);
    switch (status)
    {
    case WK_STATUS_RUNNING: printf("WK_STATUS_RUNNING\n"); break;
    case WK_STATUS_DAMAGED: printf("WK_STATUS_DAMAGED\n"); break;
    case WK_STATUS_EXIT_OK: printf("WK_STATUS_EXIT_OK\n"); break;
    case WK_STATUS_EXIT_SOFTWARE: printf("WK_STATUS_EXIT_SOFTWARE\n"); break;
    default: printf("WK_STATUS_UNKNOWN\n"); break;
    }
}

void
debugStringWithIndent(const char* text)
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
debugStringLenWithIndent(const char* text, size_t len)
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
