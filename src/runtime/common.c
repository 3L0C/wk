#include <assert.h>
#include <ctype.h>
#include <stdbool.h>

/* common includes */
#include "common/common.h"

/* local includes */
#include "common.h"

bool
isNormalKey(char* buffer, size_t len)
{
    assert(buffer);
    if (len == 0) return false;
    if (isUtf8MultiByteStartByte(*buffer)) return true;
    return isprint(*buffer) && *buffer != ' ';
}
