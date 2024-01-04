#include <stdio.h>
#include <sysexits.h>

#include "lib/common.h"
#include "lib/window.h"

int
run(WkWindow* window)
{
    printf("x11\n");
    return EX_OK;
}
