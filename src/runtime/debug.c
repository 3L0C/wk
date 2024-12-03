/* common headers */
#include "common/debug.h"

/* local header */
#include "debug.h"

void
disassembleCairoPaint(const CairoPaint* paint)
{
    /* FOREGROUND - key*/
    debugMsgWithIndent(0, "|---- Foreground Key Value ----");
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| Red:               %#02X", (uint8_t)(paint->fgKey.r * 255));
    debugMsgWithIndent(0, "| Green:             %#02X", (uint8_t)(paint->fgKey.g * 255));
    debugMsgWithIndent(0, "| Blue:              %#02X", (uint8_t)(paint->fgKey.b * 255));
    debugMsgWithIndent(0, "| Alpha:             %#02X", (uint8_t)(paint->fgKey.a * 255));
    debugMsgWithIndent(0, "|");

    /* FOREGROUND - delimiter */
    debugMsgWithIndent(0, "|--- Foreground Delim Value ---");
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| Red:               %#02X", (uint8_t)(paint->fgDelimiter.r * 255));
    debugMsgWithIndent(0, "| Green:             %#02X", (uint8_t)(paint->fgDelimiter.g * 255));
    debugMsgWithIndent(0, "| Blue:              %#02X", (uint8_t)(paint->fgDelimiter.b * 255));
    debugMsgWithIndent(0, "| Alpha:             %#02X", (uint8_t)(paint->fgDelimiter.a * 255));
    debugMsgWithIndent(0, "|");

    /* FOREGROUND - prefix */
    debugMsgWithIndent(0, "|-- Foreground Prefix Value ---");
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| Red:               %#02X", (uint8_t)(paint->fgPrefix.r * 255));
    debugMsgWithIndent(0, "| Green:             %#02X", (uint8_t)(paint->fgPrefix.g * 255));
    debugMsgWithIndent(0, "| Blue:              %#02X", (uint8_t)(paint->fgPrefix.b * 255));
    debugMsgWithIndent(0, "| Alpha:             %#02X", (uint8_t)(paint->fgPrefix.a * 255));
    debugMsgWithIndent(0, "|");

    /* FOREGROUND - chord */
    debugMsgWithIndent(0, "|--- Foreground Chord Value ---");
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| Red:               %#02X", (uint8_t)(paint->fgChord.r * 255));
    debugMsgWithIndent(0, "| Green:             %#02X", (uint8_t)(paint->fgChord.g * 255));
    debugMsgWithIndent(0, "| Blue:              %#02X", (uint8_t)(paint->fgChord.b * 255));
    debugMsgWithIndent(0, "| Alpha:             %#02X", (uint8_t)(paint->fgChord.a * 255));
    debugMsgWithIndent(0, "|");

    debugMsgWithIndent(0, "|------ Background value ------");
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| Red:               %#02X", (uint8_t)(paint->bg.r * 255));
    debugMsgWithIndent(0, "| Green:             %#02X", (uint8_t)(paint->bg.g * 255));
    debugMsgWithIndent(0, "| Blue:              %#02X", (uint8_t)(paint->bg.b * 255));
    debugMsgWithIndent(0, "| Alpha:             %#02X", (uint8_t)(paint->bg.a * 255));
    debugMsgWithIndent(0, "|");

    debugMsgWithIndent(0, "|-------- Border value --------");
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| Red:               %#02X", (uint8_t)(paint->bd.r * 255));
    debugMsgWithIndent(0, "| Green:             %#02X", (uint8_t)(paint->bd.g * 255));
    debugMsgWithIndent(0, "| Blue:              %#02X", (uint8_t)(paint->bd.b * 255));
    debugMsgWithIndent(0, "| Alpha:             %#02X", (uint8_t)(paint->bd.a * 255));
}
