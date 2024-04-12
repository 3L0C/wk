#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "debug.h"
#include "menu.h"
#include "types.h"

static char
getDelim(int* count, char a, char b)
{
    assert(count);

    return ((*count)-- > 1 ? a : b);
}

static void
printDebug(unsigned int indent)
{
    printf("[DEBUG] %*s", indent * 8, (indent ? " " : ""));
}

static void
debugInt(const char* text, int value, unsigned int indent)
{
    assert(text);

    static const int max = 20;
    int len = strlen(text);
    printDebug(indent);
    printf("| %s%*s%04d\n", text, ((max - len) > 0) ? max - len : 1, " ", value);
}

static void
debugSize_t(const char* text, size_t value, unsigned int indent)
{
    assert(text);

    static const int max = 20;
    int len = strlen(text);
    printDebug(indent);
    printf("| %s%*s%04zu\n", text, ((max - len) > 0) ? max - len : 1, " ", value);
}

static void
debugUint32_t(const char* text, uint32_t value, unsigned int indent)
{
    assert(text);

    static const int max = 20;
    int len = strlen(text);
    printDebug(indent);
    printf("| %s%*s%04u\n", text, ((max - len) > 0) ? max - len : 1, " ", value);
}

static void
debugMod(const WkMods* mods, unsigned int indent)
{
    assert(mods);

    printDebug(indent);

    if (!IS_MOD(*mods))
    {
        printf("| Mods                NONE\n");
        return;
    }

    printf("| Mods                ");
    int count = COUNT_MODS(*mods);
    if (mods->ctrl) printf("CTRL%c", getDelim(&count, '|', '\n'));
    if (mods->alt) printf("ALT%c", getDelim(&count, '|', '\n'));
    if (mods->hyper) printf("HYPER%c", getDelim(&count, '|', '\n'));
    if (mods->shift) printf("SHIFT%c", getDelim(&count, '|', '\n'));
}

static void
debugPointer(const char* text, const void* value, unsigned int indent)
{
    assert(text);

    static const int max = 20;
    int len = strlen(text);
    printDebug(indent);
    printf("| %s%*s%s\n", text, (max - len) > 0 ? max - len : 1, " ", value ? "(not null)" : "(null)");
}

static bool
debugSpecial(WkSpecial special, unsigned int indent)
{
    printDebug(indent);
    printf("| Special             ");
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
debugSpecialRepr(WkSpecial special, unsigned int indent)
{
    printDebug(indent);
    printf("| Key                 ");
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
debugString(const char* text, const char* value, unsigned int indent)
{
    assert(text);

    static const int max = 20;
    int len = strlen(text);
    printDebug(indent);
    printf("| %s%*s'%s'\n", text, ((max - len) > 0) ? max - len : 1, " ", value);
}

static void
debugFlags(const WkFlags* flags, unsigned int indent)
{
    assert(flags);

    printDebug(indent);

    if (!HAS_FLAG(*flags))
    {
        printf("| Flags:              WK_FLAG_DEFAULTS\n");
        return;
    }

    printf("| Flags:              ");
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
debugKeyChord(const WkKeyChord* keyChord, unsigned int indent)
{
    assert(keyChord);

    printDebug(indent);

    printf("------------------ Chord -------------------\n");

    debugMod(&keyChord->mods, indent);
    debugSpecial(keyChord->special, indent);
    debugString("Key", keyChord->key, indent);
    debugString("Description", keyChord->description, indent);
    debugString("Hint", keyChord->hint, indent);
    debugString("Command", keyChord->command, indent);
    debugString("Before", keyChord->before, indent);
    debugString("After", keyChord->after, indent);
    debugFlags(&keyChord->flags, indent);
    debugPointer("Chords", keyChord->keyChords, indent);

    printDebug(indent);
    printf("--------------------------------------------\n");
}

void
debugKeyChords(const WkKeyChord* keyChords, unsigned int indent)
{
    assert(keyChords);

    for (uint32_t i = 0; keyChords[i].key; i++)
    {
        debugKeyChord(&keyChords[i], indent);
        if (keyChords[i].keyChords)
        {
            debugKeyChords(keyChords[i].keyChords, indent + 1);
        }
    }
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
    printDebug(0);
    printf("------------------- Grid -------------------\n");
    debugUint32_t("Start X:", startx, 0);
    debugUint32_t("Start Y:", starty, 0);
    debugUint32_t("Rows:", rows, 0);
    debugUint32_t("Columns:", cols, 0);
    debugUint32_t("Width padding:", wpadding, 0);
    debugUint32_t("Height padding:", hpadding, 0);
    debugUint32_t("Cell width:", cellw, 0);
    debugUint32_t("Cell height:", cellh, 0);
    debugUint32_t("Count:", count, 0);
    printDebug(0);
    printf("--------------------------------------------\n");
}

static void
debugHexColor(const WkHexColor* color)
{
    assert(color);

    printf("[DEBUG] |     Hex string:     '%s'\n", color->hex);
    printf("[DEBUG] |     Red value:      %#02X\n", color->r * 255);
    printf("[DEBUG] |     Green value:    %#02X\n", color->g * 255);
    printf("[DEBUG] |     Blue value:     %#02X\n", color->b * 255);
    printf("[DEBUG] |     Alpha value:    %#02X\n", color->a * 255);
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
        case WK_COLOR_FOREGROUND: printf("| Foreground color:\n"); break;
        case WK_COLOR_BACKGROUND: printf("| Background color:\n"); break;
        case WK_COLOR_BORDER: printf("| Border color:\n"); break;
        default: errorMsg("| Got unexpected color index: '%d'.", i); return;
        }

        debugHexColor(&colors[i]);
    }
}

void
debugKey(const WkKey* key)
{
    assert(key);

    printDebug(0);
    printf("------------------- Key --------------------\n");
    debugMod(&key->mods, 0);
    if (debugSpecial(key->special, 0))
    {
        /* printDebug(0); */
        debugSpecialRepr(key->special, 0);
        /* printf("| Key                 SPECIAL\n"); */
    }
    else
    {
        debugString("Key", key->key, 0);
    }
    debugInt("len", key->len, 0);
    printDebug(0);
    printf("--------------------------------------------\n");
}

void
debugMenu(const WkMenu* menu)
{
    assert(menu);

    printDebug(0);
    printf("------------------- Menu -------------------\n");
    debugString("Delimiter:", menu->delimiter, 0);
    debugSize_t("Max cols:", menu->maxCols, 0);
    debugInt("Window width:", menu->windowWidth, 0);
    debugInt("Window gap:", menu->windowGap, 0);
    debugUint32_t("Width padding:", menu->wpadding, 0);
    debugUint32_t("Height padding:", menu->hpadding, 0);
    debugUint32_t("Cell height:", menu->cellHeight, 0);
    debugUint32_t("Rows:", menu->rows, 0);
    debugUint32_t("Cols:", menu->cols, 0);
    debugUint32_t("Width:", menu->width, 0);
    debugUint32_t("Height:", menu->height, 0);
    debugString(
        "Window position:",
        (menu->position == WK_WIN_POS_BOTTOM ? "Bottom" : "Top"),
        0
    );
    debugSize_t("Border width:", menu->borderWidth, 0);
    debugHexColors(menu->colors);
    debugString("Shell:", menu->shell, 0);
    debugString("Font:", menu->font, 0);
    debugString("Chords:", "Set", 0);
    debugUint32_t("Chord count:", menu->keyChordCount, 0);
    debugString("Debug:", "True", 0);
    debugString("Keys:", menu->client.keys, 0);
    debugString("Transpile:", menu->client.transpile, 0);
    debugString("Chords file:", menu->client.keyChordsFile, 0);
    debugString("Try script:", (menu->client.tryScript ? "True" : "False"), 0);
    debugString("Script:", menu->client.script.string, 0);
    debugSize_t("Script capacity:", menu->client.script.capacity, 0);
    debugSize_t("Script count:", menu->client.script.count, 0);
    printDebug(0);
    printf("--------------------------------------------\n");
}

void
debugMsg(bool debug, const char* fmt, ...)
{
    if (!debug) return;

    assert(fmt);

    static const int debugLen = strlen("[DEBUG] ");

    int len = strlen(fmt) + 1; /* 1 = '\0' */
    char format[len + debugLen];
    memcpy(format, "[DEBUG] ", debugLen);
    memcpy(format + debugLen, fmt, len);
    va_list ap;
    va_start(ap, fmt);
    vprintf(format, ap);
    va_end(ap);

    fputc((fmt[len - 1] == ':' ? ' ' : '\n'), stdout);
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
    if (!text)
    {
        printf("      | ");
    }
    const char* current = text;

    while (*current != '\0')
    {
        printf("      | ");
        while (*current != '\n' && *current != '\0')
        {
            printf("%c", *current++);
        }
        printf("%c", *current++);
    }
}

void
debugStringLenWithIndent(const char* text, size_t len)
{
    if (!text) return;

    const char* current = text;

    while (current < text + len)
    {
        printf("      | ");
        while (*current != '\n' && current < text + len)
        {
            printf("%c", *current++);
        }
        if (*current == '\n') printf("%c", *current++);
    }
}
