#include <stdbool.h>

/* local includes */
#include "modifier.h"

int
modifierCount(Modifier mod)
{
    int result = 0;
    while (mod)
    {
        result++;
        mod &= mod - 1;
    }

    return result;
}

bool
modifierHasAnyActive(Modifier mod)
{
    static const Modifier any =
        MOD_CTRL |
        MOD_META |
        MOD_HYPER |
        MOD_SHIFT;

    return (mod & any) != 0;
}

Modifier
modifierInit(void)
{
    return MOD_NONE;
}

bool
modifierIsActive(Modifier mod, Modifier test)
{
    return (mod & test) != 0;
}
