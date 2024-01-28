#include <stdio.h>

#include "debug.h"
#include "lib/debug.h"
#include "window.h"

static void
debugRootDispaly(struct display* root)
{
    printf("Root x:             %u\n", root->x);
    printf("Root y:             %u\n", root->y);
    printf("Root width:         %u\n", root->w);
    printf("Root height:        %u\n", root->h);
}

void
debugWindow(WkX11Window* window)
{
    printf("--- Beg WkX11Window ---\n");
    printf("Window x:           %u\n", window->x);
    printf("Window y:           %u\n", window->y);
    printf("Window width:       %u\n", window->width);
    printf("Window height:      %u\n", window->height);
    printf("Window border:      %u\n", window->border);
    printf("Window max height:  %u\n", window->maxHeight);
    debugRootDispaly(&window->root);
    debugCairoPaint(&window->paint);
    printf("--- End WkX11Window ---\n");
}
