#include <stdbool.h>

/* local includes */
#include "chord_flag.h"

int
chordFlagCount(ChordFlag flag)
{
    int result = 0;
    while (flag)
    {
        result++;
        flag &= flag - 1;
    }

    return result;
}

bool
chordFlagHasAnyActive(ChordFlag flag)
{
    static const ChordFlag any =
        FLAG_KEEP |
        FLAG_CLOSE |
        FLAG_INHERIT |
        FLAG_IGNORE |
        FLAG_UNHOOK |
        FLAG_DEFLAG |
        FLAG_NO_BEFORE |
        FLAG_NO_AFTER |
        FLAG_WRITE |
        FLAG_EXECUTE |
        FLAG_SYNC_COMMAND |
        FLAG_SYNC_BEFORE |
        FLAG_SYNC_AFTER |
        FLAG_UNWRAP;

    return (flag & any) != 0;
}

ChordFlag
chordFlagInit(void)
{
    return FLAG_NONE;
}

bool
chordFlagIsActive(ChordFlag flag, ChordFlag test)
{
    return (flag & test) != 0;
}

bool
chordFlagsAreDefault(ChordFlag flag)
{
    return !chordFlagHasAnyActive(flag);
}
