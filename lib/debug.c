#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "client.h"
#include "common.h"
#include "debug.h"
#include "properties.h"
#include "types.h"
#include "window.h"

static char
getDelim(int* count, char a, char b)
{
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
    static const int max = 20;
    int len = strlen(text);
    printDebug(indent);
    printf("| %s%*s%04d\n", text, ((max - len) > 0) ? max - len : 1, " ", value);
}

static void
debugSize_t(const char* text, size_t value, unsigned int indent)
{
    static const int max = 20;
    int len = strlen(text);
    printDebug(indent);
    printf("| %s%*s%04zu\n", text, ((max - len) > 0) ? max - len : 1, " ", value);
}

static void
debugUint32_t(const char* text, uint32_t value, unsigned int indent)
{
    static const int max = 20;
    int len = strlen(text);
    printDebug(indent);
    printf("| %s%*s%04u\n", text, ((max - len) > 0) ? max - len : 1, " ", value);
}

static void
debugMod(const WkMods* mods, unsigned int indent)
{
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

static void
debugString(const char* text, const char* value, unsigned int indent)
{
    static const int max = 20;
    int len = strlen(text);
    printDebug(indent);
    printf("| %s%*s'%s'\n", text, ((max - len) > 0) ? max - len : 1, " ", value);
}

static void
debugFlags(const WkFlags* flags, unsigned int indent)
{
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
    if (flags->unhook) printf("UNHOOK%c", getDelim(&count, '|', '\n'));
    if (flags->nobefore) printf("NOBEFORE%c", getDelim(&count, '|', '\n'));
    if (flags->noafter) printf("NOAFTER%c", getDelim(&count, '|', '\n'));
    if (flags->write) printf("WRITE%c", getDelim(&count, '|', '\n'));
    if (flags->syncCommand) printf("SYNCCOMMAND%c", getDelim(&count, '|', '\n'));
    if (flags->beforeAsync) printf("BEFOREASYNC%c", getDelim(&count, '|', '\n'));
    if (flags->afterSync) printf("AFTERSYNC%c", getDelim(&count, '|', '\n'));
}

void
debugCairoPaint(const CairoPaint* paint)
{
    /* FOREGROUND */
    printf("[DEBUG] |     - Foreground value -\n");
    printf("[DEBUG] |     Red:            %#02X\n", (uint8_t)(paint->fg.r * 255));
    printf("[DEBUG] |     Green:          %#02X\n", (uint8_t)(paint->fg.g * 255));
    printf("[DEBUG] |     Blue:           %#02X\n", (uint8_t)(paint->fg.b * 255));
    printf("[DEBUG] |     Alpha:          %#02X\n", (uint8_t)(paint->fg.a * 255));

    /* BACKGROUND */
    printf("[DEBUG] |     - Background value -\n");
    printf("[DEBUG] |     Red:            %#02X\n", (uint8_t)(paint->bg.r * 255));
    printf("[DEBUG] |     Green:          %#02X\n", (uint8_t)(paint->bg.g * 255));
    printf("[DEBUG] |     Blue:           %#02X\n", (uint8_t)(paint->bg.b * 255));
    printf("[DEBUG] |     Alpha:          %#02X\n", (uint8_t)(paint->bg.a * 255));

    /* BORDER */
    printf("[DEBUG] |     --- Border value ---\n");
    printf("[DEBUG] |     Red:            %#02X\n", (uint8_t)(paint->bd.r * 255));
    printf("[DEBUG] |     Green:          %#02X\n", (uint8_t)(paint->bd.g * 255));
    printf("[DEBUG] |     Blue:           %#02X\n", (uint8_t)(paint->bd.b * 255));
    printf("[DEBUG] |     Alpha:          %#02X\n", (uint8_t)(paint->bd.a * 255));
}

void
debugChord(const Chord* chord, unsigned int indent)
{
    printDebug(indent);

    printf("------------------ Chord -------------------\n");

    debugMod(&chord->mods, indent);
    debugSpecial(chord->special, indent);
    debugString("Key", chord->key, indent);
    debugString("Description", chord->description, indent);
    debugString("Hint", chord->hint, indent);
    debugString("Command", chord->command, indent);
    debugString("Before", chord->before, indent);
    debugString("After", chord->after, indent);
    debugFlags(&chord->flags, indent);
    debugPointer("Chords", chord->chords, indent);

    printDebug(indent);
    printf("--------------------------------------------\n");
}

void
debugChords(const Chord* chords, unsigned int indent)
{
    for (uint32_t i = 0; chords[i].key; i++)
    {
        debugChord(&chords[i], indent);
        if (chords[i].chords)
        {
            debugChords(chords[i].chords, indent + 1);
        }
    }
}

void
debugChordsShallow(const Chord* chords, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        debugChord(&chords[i], 0);
    }
}

void
debugClient(const Client* client)
{
    printDebug(0);
    printf("------------------ Client ------------------\n");
    debugString("Delimiter:", client->delimiter, 0);
    debugSize_t("Max cols:", client->maxCols, 0);
    debugInt("Window width:", client->windowWidth, 0);
    debugInt("Window gap:", client->windowGap, 0);
    debugUint32_t("Width padding:", client->wpadding, 0);
    debugUint32_t("Height padding:", client->hpadding, 0);
    debugString(
        "Window position:",
        (client->windowPosition == WK_WIN_POS_BOTTOM ? "Bottom" : "Top"),
        0
    );
    debugSize_t("Border width:", client->borderWidth, 0);
    debugString("Foreground:", client->foreground, 0);
    debugString("Background:", client->background, 0);
    debugString("Border:", client->border, 0);
    debugString("Shell:", client->shell, 0);
    debugString("Font:", client->font, 0);
    debugString("Keys:", client->keys, 0);
    debugString("Transpile:", client->transpile, 0);
    debugString("Chords file:", client->chordsFile, 0);
    debugString("Try script:", (client->tryScript ? "True" : "False"), 0);
    debugString("Script:", client->script, 0);
    debugSize_t("Script capacity:", client->scriptCapacity, 0);
    debugSize_t("Script count:", client->scriptCount, 0);
    debugString("Chords:", "Set", 0);
    debugString("Debug:", "True", 0);
    printDebug(0);
    printf("--------------------------------------------\n");
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
    printf("[DEBUG] |     Hex string:     '%s'\n", color->hex);
    printf("[DEBUG] |     Red value:      %#02X\n", color->r * 255);
    printf("[DEBUG] |     Green value:    %#02X\n", color->g * 255);
    printf("[DEBUG] |     Blue value:     %#02X\n", color->b * 255);
    printf("[DEBUG] |     Alpha value:    %#02X\n", color->a * 255);
}

void
debugHexColors(const WkHexColor* colors)
{
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
debugKey(const Key* key)
{
    printDebug(0);
    printf("------------------- Key --------------------\n");
    debugMod(&key->mods, 0);
    if (debugSpecial(key->special, 0))
    {
        printDebug(0);
        printf("| Key                 SPECIAL\n");
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
debugProperties(const WkProperties* props)
{
    printDebug(0);
    printf("---------------- Properties ----------------\n");
    debugString("Delimiter:", props->delimiter, 0);
    debugSize_t("Max cols:", props->maxCols, 0);
    debugInt("Window width:", props->windowWidth, 0);
    debugInt("Window gap:", props->windowGap, 0);
    debugUint32_t("Width padding:", props->wpadding, 0);
    debugUint32_t("Height padding:", props->hpadding, 0);
    debugUint32_t("Cell height:", props->cellHeight, 0);
    debugUint32_t("Rows:", props->rows, 0);
    debugUint32_t("Cols:", props->cols, 0);
    debugUint32_t("Width:", props->width, 0);
    debugUint32_t("Height:", props->height, 0);
    debugString(
        "Window position:",
        (props->position == WK_WIN_POS_BOTTOM ? "Bottom" : "Top"),
        0
    );
    debugSize_t("Border width:", props->borderWidth, 0);
    debugHexColors(props->colors);
    debugString("Shell:", props->shell, 0);
    debugString("Font:", props->font, 0);
    debugString("Chords:", "Set", 0);
    debugUint32_t("Chord count:", props->chordCount, 0);
    debugString("Debug:", "True", 0);
    printDebug(0);
    printf("--------------------------------------------\n");
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
