#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "debug.h"
#include "menu.h"
#include "key_chord.h"
#include "util.h"

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
        case MENU_COLOR_FOREGROUND: debugMsg(true, "|------- Foreground color -------"); break;
        case MENU_COLOR_BACKGROUND: debugMsg(true, "|------- Background color -------"); break;
        case MENU_COLOR_BORDER:     debugMsg(true, "|--------- Border color ---------"); break;
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
disassembleMod(const Modifiers* mods, int indent)
{
    assert(mods);

    debugMsgWithIndent(indent, "| Mods:              ");

    if (!hasActiveModifier(mods))
    {
        printf("NONE\n");
        return;
    }

    int count = countModifiers(mods);
    if (mods->ctrl) printf("CTRL%c", getDelim(&count, '|', '\n'));
    if (mods->alt) printf("ALT%c", getDelim(&count, '|', '\n'));
    if (mods->hyper) printf("HYPER%c", getDelim(&count, '|', '\n'));
    if (mods->shift) printf("SHIFT%c", getDelim(&count, '|', '\n'));
}

static bool
disassembleSpecial(SpecialKey special, int indent)
{
    debugMsgWithIndent(indent, "| Special:           ");
    const char* text = getSpecialKeyLiteral(special);
    bool flag = special == SPECIAL_KEY_NONE ? false : true;

    printf("%s|%d\n", text, special);
    return flag;
}

static void
debugString(const char* text, const char* value, int indent)
{
    assert(text);

    debugMsgWithIndent(indent, "| %-19s'%s'", text, value);
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

    disassembleMod(&key->mods, indent);
    if (disassembleSpecial(key->special, indent))
    {
        debugString("Key:", getSpecialKeyRepr(key->special), indent);
    }
    else
    {
        debugString("Key:", key->repr, indent);
    }
    debugMsgWithIndent(indent, "| Length:            %04d", key->len);
}

void
disassembleFlags(const ChordFlags* flags, int indent)
{
    assert(flags);

    debugMsgWithIndent(indent, "| Flags:             ");

    if (!hasChordFlags(flags))
    {
        printf("WK_FLAG_DEFAULTS\n");
        return;
    }

    int count = countChordFlags(flags);
    if (flags->keep) printf("KEEP%c", getDelim(&count, '|', '\n'));
    if (flags->close) printf("CLOSE%c", getDelim(&count, '|', '\n'));
    if (flags->inherit) printf("INHERIT%c", getDelim(&count, '|', '\n'));
    if (flags->ignore) printf("IGNORE%c", getDelim(&count, '|', '\n'));
    if (flags->unhook) printf("UNHOOK%c", getDelim(&count, '|', '\n'));
    if (flags->deflag) printf("DEFLAG%c", getDelim(&count, '|', '\n'));
    if (flags->nobefore) printf("NO_BEFORE%c", getDelim(&count, '|', '\n'));
    if (flags->noafter) printf("NO_AFTER%c", getDelim(&count, '|', '\n'));
    if (flags->write) printf("WRITE%c", getDelim(&count, '|', '\n'));
    if (flags->execute) printf("EXECUTE%c", getDelim(&count, '|', '\n'));
    if (flags->syncCommand) printf("SYNC_COMMAND%c", getDelim(&count, '|', '\n'));
    if (flags->syncBefore) printf("BEFORE_SYNC%c", getDelim(&count, '|', '\n'));
    if (flags->syncAfter) printf("AFTER_SYNC%c", getDelim(&count, '|', '\n'));
}

void
disassembleKeyChord(const KeyChord* keyChord, int indent)
{
    assert(keyChord);

    disassembleKeyWithoutHeader(&keyChord->key, indent);
    debugMsgWithIndent(indent, "| Description:       \"%s\"", keyChord->description);
    debugMsgWithIndent(indent, "| Hint:              '%s'", keyChord->hint);
    debugMsgWithIndent(indent, "| Command:           %{{ %s }}", keyChord->command);
    debugMsgWithIndent(indent, "| Before:            %{{ %s }}", keyChord->before);
    debugMsgWithIndent(indent, "| After:             %{{ %s }}", keyChord->after);
    disassembleFlags(&keyChord->flags, indent);
}

void
disassembleKeyChords(const KeyChord* keyChords, int indent)
{
    assert(keyChords);

    if (indent == 0)
    {
        debugPrintHeaderWithIndent(indent, " KeyChords ");
    }
    for (uint32_t i = 0; keyChords[i].state == KEY_CHORD_STATE_NOT_NULL; i++)
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
            disassembleKeyChords(keyChords[i].keyChords, indent + 1);
        }
        debugPrintHeaderWithIndent(indent, "-");
    }
    if (indent == 0) printf("\n");
}

void
disassembleKeyChordsShallow(const KeyChord* keyChords, uint32_t len)
{
    assert(keyChords);

    for (uint32_t i = 0; i < len; i++)
    {
        disassembleKeyChordWithHeader(&keyChords[i], 0);
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
        (menu->position == MENU_WIN_POS_BOTTOM ? "BOTTOM" : "TOP")
    );
    debugMsgWithIndent(0, "| Border width:      %04u",  menu->borderWidth);
    debugMsgWithIndent(0, "|");
    disassembleHexColors(menu->colors);
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| Shell:             '%s'",  menu->shell);
    debugMsgWithIndent(0, "| Font:              '%s'",  menu->font);
    debugMsgWithIndent(0, "| Chords:            %p",    menu->keyChords);
    debugMsgWithIndent(0, "| Chord count:       %04u",  menu->keyChordCount);
    debugMsgWithIndent(0, "| Debug:             %s",    "true");
    debugMsgWithIndent(0, "| Keys:              %s",    menu->client.keys);
    debugMsgWithIndent(0, "| Transpile:         %s",    menu->client.transpile);
    debugMsgWithIndent(0, "| wks file:          '%s'",  menu->client.wksFile);
    debugMsgWithIndent(0, "| Try script:        %s",    (menu->client.tryScript ? "true" : "false"));
    if (menu->client.script.string)
    {
        debugMsgWithIndent(0, "| Script:");
        debugMsgWithIndent(0, "|");
        debugTextWithLineNumber(menu->client.script.string);
        debugMsgWithIndent(0, "|");
    }
    else
    {
        debugMsgWithIndent(0, "| Script:            (null)");
    }
    debugMsgWithIndent(0, "| Script capacity:   %04zu", menu->client.script.capacity);
    debugMsgWithIndent(0, "| Script count:      %04zu", menu->client.script.count);
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

