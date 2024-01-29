#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "client.h"
#include "common.h"
#include "debug.h"
#include "properties.h"
#include "types.h"
#include "util.h"

static void
debugInt(const char* text, int value)
{
    static const int max = 20;
    int len = strlen(text);
    printf("| %s%*s%04d\n", text, ((max - len) > 0) ? max - len : 1, " ", value);
}

static void
debugMod(WkMod mod)
{
    if (!IS_MOD(mod))
    {
        printf("| Mods                WK_MOD_NONE|%u\n", mod);
        return;
    }

    printf("| Mods                ");
    const char* mods[4];
    int idx = 0;
    if (IS_CTRL(mod)) mods[idx++] = "CTRL";
    if (IS_ALT(mod)) mods[idx++] = "ALT";
    if (IS_HYPER(mod)) mods[idx++] = "HYPER";
    if (IS_SHIFT(mod)) mods[idx++] = "SHIFT";

    for (int i = 0; i < idx; i++)
    {
        printf("%s|", mods[i]);
    }
    printf("%u\n", mod);
}

static void
debugPointer(const char* text, const void* value)
{
    static const int max = 20;
    int len = strlen(text);
    printf("| %s%*s%s\n", text, (max - len) > 0 ? max - len : 1, " ", value ? "(not null)" : "(null)");
}

static bool
debugSpecial(SpecialType special)
{
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
debugString(const char* text, const char* value)
{
    static const int max = 20;
    int len = strlen(text);
    printf("| %s%*s'%s'\n", text, ((max - len) > 0) ? max - len : 1, " ", value);
}

static void
debugFlags(WkFlags flags)
{
    if (flags == 0)
    {
        printf("| Flags:              WK_FLAG_DEFAULTS|%d\n", flags);
        return;
    }

    printf("| Flags:              ");
    if (IS_FLAG(flags, WK_FLAG_KEEP)) printf("WK_FLAG_KEEP|");
    if (IS_FLAG(flags, WK_FLAG_UNHOOK)) printf("WK_FLAG_UNHOOK|");
    if (IS_FLAG(flags, WK_FLAG_NOBEFORE)) printf("WK_FLAG_NOBEFORE|");
    if (IS_FLAG(flags, WK_FLAG_NOAFTER)) printf("WK_FLAG_NOAFTER|");
    if (IS_FLAG(flags, WK_FLAG_WRITE)) printf("WK_FLAG_WRITE|");
    if (IS_FLAG(flags, WK_FLAG_SYNC_COMMAND)) printf("WK_FLAG_SYNC_COMMAND|");
    if (IS_FLAG(flags, WK_FLAG_BEFORE_ASYNC)) printf("WK_FLAG_BEFORE_ASYNC|");
    if (IS_FLAG(flags, WK_FLAG_AFTER_SYNC)) printf("WK_FLAG_AFTER_SYNC|");
    printf("%d\n", flags);
}

void
debugCairoColor(const CairoColor* color)
{
    printf("|     Red:            %#02X\n", (uint8_t)(color->r * 255));
    printf("|     Green:          %#02X\n", (uint8_t)(color->g * 255));
    printf("|     Blue:           %#02X\n", (uint8_t)(color->b * 255));
    printf("|     Alpha:          %#02X\n", (uint8_t)(color->a * 255));
}

void
debugCairoPaint(const CairoPaint* paint)
{
    /* FOREGROUND */
    printf("|     - Foreground value -\n");
    printf("|     Red:            %#02X\n", (uint8_t)(paint->fg.r * 255));
    printf("|     Green:          %#02X\n", (uint8_t)(paint->fg.g * 255));
    printf("|     Blue:           %#02X\n", (uint8_t)(paint->fg.b * 255));
    printf("|     Alpha:          %#02X\n", (uint8_t)(paint->fg.a * 255));

    /* BACKGROUND */
    printf("|     - Background value -\n");
    printf("|     Red:            %#02X\n", (uint8_t)(paint->bg.r * 255));
    printf("|     Green:          %#02X\n", (uint8_t)(paint->bg.g * 255));
    printf("|     Blue:           %#02X\n", (uint8_t)(paint->bg.b * 255));
    printf("|     Alpha:          %#02X\n", (uint8_t)(paint->bg.a * 255));

    /* BORDER */
    printf("|     --- Border value ---\n");
    printf("|     Red:            %#02X\n", (uint8_t)(paint->bd.r * 255));
    printf("|     Green:          %#02X\n", (uint8_t)(paint->bd.g * 255));
    printf("|     Blue:           %#02X\n", (uint8_t)(paint->bd.b * 255));
    printf("|     Alpha:          %#02X\n", (uint8_t)(paint->bd.a * 255));
}

void
debugChord(const Chord* chord)
{
    printf("------------------ Chord -------------------\n");
    debugMod(chord->mods);
    debugSpecial(chord->special);
    debugString("Key", chord->key);
    debugString("Description", chord->description);
    debugString("Hint", chord->hint);
    debugString("Command", chord->command);
    debugString("Before", chord->before);
    debugString("After", chord->after);
    debugFlags(chord->flags);
    debugPointer("Chords", chord->chords);
    printf("--------------------------------------------\n");
}

void
debugClient(const Client* client)
{
    printf("------------------ Client ------------------\n");
    printf("| Delimiter:          '%s'\n", client->delimiter);
    printf("| Max cols:           %zu\n", client->maxCols);
    printf("| Window width:       %d\n", client->windowWidth);
    printf("| Window gap:         %d\n", client->windowGap);
    printf("| Width padding:      %u\n", client->wpadding);
    printf("| Height padding:     %u\n", client->hpadding);
    printf("| Window position:    %zu\n", client->windowPosition);
    printf("| Border width:       %zu\n", client->borderWidth);
    printf("| Foreground:         '%s'\n", client->foreground);
    printf("| Background:         '%s'\n", client->background);
    printf("| Border:             '%s'\n", client->border);
    printf("| Shell:              '%s'\n", client->shell);
    printf("| Font:               '%s'\n", client->font);
    printf("| Keys:               '%s'\n", client->keys);
    printf("| Transpile:          '%s'\n", client->transpile);
    printf("| Chords file:        '%s'\n", client->chordsFile);
    printf("| Try script:         %s\n", client->tryScript ? "True" : "False");
    printf("| Script:             '%s'\n", client->script);
    printf("| Script capacity:    %zu\n", client->scriptCapacity);
    printf("| Script count:       %zu\n", client->scriptCount);
    printf("| Chords:             Set\n");
    printf("| Debug:              True\n");
    printf("--------------------------------------------\n");
}

static void
debugHexColor(const WkHexColor* color)
{
    printf("|     Hex string:     '%s'\n", color->hex);
    printf("|     Red value:      %#02X\n", color->r * 255);
    printf("|     Green value:    %#02X\n", color->g * 255);
    printf("|     Blue value:     %#02X\n", color->b * 255);
    printf("|     Alpha value:    %#02X\n", color->a * 255);
}

void
debugHexColors(const WkHexColor* colors)
{
    for (int i = 0; i < WK_COLOR_LAST; i++)
    {
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
    printf("------------------- Key --------------------\n");
    debugMod(key->mods);
    if (debugSpecial(key->special))
    {
        printf("| Key                 SPECIAL\n");
    }
    else
    {
        debugString("Key", key->key);
    }
    debugInt("len", key->len);
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
    vfprintf(stderr, format, ap);
    va_end(ap);

    fputc((fmt[len - 1] == ':' ? ' ' : '\n'), stderr);
}

void
debugProperties(const WkProperties* props)
{
    printf("---------------- Properties ----------------\n");
    printf("| Delimiter:          '%s'\n", props->delimiter);
    printf("| Max cols:           %zu\n", props->maxCols);
    printf("| Window width:       %d\n", props->windowWidth);
    printf("| Window gap:         %d\n", props->windowGap);
    printf("| Width padding:      %u\n", props->wpadding);
    printf("| Height padding:     %u\n", props->hpadding);
    printf("| Cell height:        %u\n", props->cell_height);
    printf("| Rows:               %u\n", props->rows);
    printf("| Cols:               %u\n", props->cols);
    printf("| Width:              %u\n", props->width);
    printf("| Height:             %u\n", props->height);
    printf("| Window position:    %s\n", (props->position == WK_WIN_POS_BOTTOM) ? "Bottom" : "Top");
    printf("| Border width:       %zu\n", props->borderWidth);
    debugHexColors(props->colors);
    printf("| Shell:              '%s'\n", props->shell);
    printf("| Font:               '%s'\n", props->font);
    printf("| Chords:             Set\n");
    printf("| Chord count:        %u\n", props->chordCount);
    printf("| Debug:              True\n");
    printf("--------------------------------------------\n");
}
