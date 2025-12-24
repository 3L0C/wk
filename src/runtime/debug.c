#include <assert.h>

/* common headers */
#include "common/debug.h"

/* local header */
#include "cairo.h"
#include "debug.h"

static void
disassembleCairoColor(int indent, const CairoColor* color)
{
    assert(color);

    debugMsgWithIndent(indent, "| Red:               %#02X", (uint8_t)(color->r * 255));
    debugMsgWithIndent(indent, "| Green:             %#02X", (uint8_t)(color->g * 255));
    debugMsgWithIndent(indent, "| Blue:              %#02X", (uint8_t)(color->b * 255));
    debugMsgWithIndent(indent, "| Alpha:             %#02X", (uint8_t)(color->a * 255));
}

static void
disassembleCairoColorWithHeader(const char* header, const CairoColor* color)
{
    assert(header), assert(color);

    debugMsgWithIndent(0, header);
    debugMsgWithIndent(0, "|");
    disassembleCairoColor(0, color);
    debugMsgWithIndent(0, "|");
}

void
disassembleCairoPaint(const CairoPaint* paint)
{
    assert(paint);

    disassembleCairoColorWithHeader("|-- Foreground Key Value ------", &paint->fgKey);
    disassembleCairoColorWithHeader("|-- Foreground Delimiter Value ", &paint->fgDelimiter);
    disassembleCairoColorWithHeader("|-- Foreground Prefix Value ---", &paint->fgPrefix);
    disassembleCairoColorWithHeader("|-- Foreground Chord Value ----", &paint->fgChord);
    disassembleCairoColorWithHeader("|-- Foreground Title Value ----", &paint->fgTitle);
    disassembleCairoColorWithHeader("|-- Foreground Goto Value -----", &paint->fgGoto);
    disassembleCairoColorWithHeader("|-- Background value ----------", &paint->bg);
    disassembleCairoColorWithHeader("|-- Border value --------------", &paint->bd);
}
