/* common includes */
#include "common/debug.h"

/* runtime includes */
#include "runtime/debug.h"

/* local includes */
#include "debug.h"
#include "window.h"

void
disassembleWaylandWindow(WaylandWindow* window)
{
    debugMsg(true, "");
    debugPrintHeader("WkWaylandWindow");
    debugMsgWithIndent(0, "|");
    debugMsgWithIndent(0, "| Window width:      %04u", window->width);
    debugMsgWithIndent(0, "| Window max width:  %04u", window->maxWidth);
    debugMsgWithIndent(0, "| Window height:     %04u", window->height);
    debugMsgWithIndent(0, "| Window max height: %04u", window->maxHeight);
    debugMsgWithIndent(0, "|");
    disassembleCairoPaint(&window->paint);
    debugMsgWithIndent(0, "|");
    debugPrintHeader("");
    debugMsg(true, "");
}
