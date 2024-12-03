/* common includes */
#include "common/debug.h"

/* runtime includes */
#include "runtime/debug.h"

/* local includes */
#include "debug.h"
#include "window.h"

static void
debugRootDispaly(struct display* root)
{
    debugMsgWithIndent(0, "| Root x:            %04u", root->x);
    debugMsgWithIndent(0, "| Root y:            %04u", root->y);
    debugMsgWithIndent(0, "| Root width:        %04u", root->w);
    debugMsgWithIndent(0, "| Root height:       %04u", root->h);
}

void
disassembleX11Window(X11Window* window)
{
    debugPrintHeader(" WkX11Window ");
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| Window x:          %04u", window->x);
    debugMsgWithIndent(0, "| Window y:          %04u", window->y);
    debugMsgWithIndent(0, "| Window width:      %04u", window->width);
    debugMsgWithIndent(0, "| Window height:     %04u", window->height);
    debugMsgWithIndent(0, "| Window border:     %04u", window->border);
    debugMsgWithIndent(0, "| Window max height: %04u", window->maxHeight);
    debugRootDispaly(&window->root);
    debugMsgWithIndent(0, "|");
    disassembleCairoPaint(&window->paint);
    debugMsgWithIndent(0, "|");
    debugPrintHeader("");
}
