#include <stdio.h>

/* runtime includes */
#include "runtime/debug.h"

/* local includes */
#include "debug.h"
#include "window.h"

static void
debugRootDispaly(struct display* root)
{
    printf("[DEBUG] | Root x:             %u\n", root->x);
    printf("[DEBUG] | Root y:             %u\n", root->y);
    printf("[DEBUG] | Root width:         %u\n", root->w);
    printf("[DEBUG] | Root height:        %u\n", root->h);
}

void
debugWindow(WkX11Window* window)
{
    printf("[DEBUG] --------------- WkX11Window ----------------\n");
    printf("[DEBUG] | Window x:           %u\n", window->x);
    printf("[DEBUG] | Window y:           %u\n", window->y);
    printf("[DEBUG] | Window width:       %u\n", window->width);
    printf("[DEBUG] | Window height:      %u\n", window->height);
    printf("[DEBUG] | Window border:      %u\n", window->border);
    printf("[DEBUG] | Window max height:  %u\n", window->maxHeight);
    debugRootDispaly(&window->root);
    debugCairoPaint(&window->paint);
    printf("[DEBUG] --------------------------------------------\n");
}
