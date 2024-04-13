/* common headers */
#include "common/debug.h"

/* local header */
#include "debug.h"

void
debugCairoPaint(const CairoPaint* paint)
{
    /* FOREGROUND */
    debugMsgWithIndent(0, "|--- Foreground value ---");
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| Red:               %#02X", (uint8_t)(paint->fg.r * 255));
    debugMsgWithIndent(0, "| Green:             %#02X", (uint8_t)(paint->fg.g * 255));
    debugMsgWithIndent(0, "| Blue:              %#02X", (uint8_t)(paint->fg.b * 255));
    debugMsgWithIndent(0, "| Alpha:             %#02X", (uint8_t)(paint->fg.a * 255));
    debugMsgWithIndent(0, "|");

    debugMsgWithIndent(0, "|--- Background value ---");
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| Red:               %#02X", (uint8_t)(paint->bg.r * 255));
    debugMsgWithIndent(0, "| Green:             %#02X", (uint8_t)(paint->bg.g * 255));
    debugMsgWithIndent(0, "| Blue:              %#02X", (uint8_t)(paint->bg.b * 255));
    debugMsgWithIndent(0, "| Alpha:             %#02X", (uint8_t)(paint->bg.a * 255));
    debugMsgWithIndent(0, "|");

    debugMsgWithIndent(0, "|----- Border value -----");
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| Red:               %#02X", (uint8_t)(paint->bd.r * 255));
    debugMsgWithIndent(0, "| Green:             %#02X", (uint8_t)(paint->bd.g * 255));
    debugMsgWithIndent(0, "| Blue:              %#02X", (uint8_t)(paint->bd.b * 255));
    debugMsgWithIndent(0, "| Alpha:             %#02X", (uint8_t)(paint->bd.a * 255));
}
