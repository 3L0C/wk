#ifndef WK_LIB_WINDOW_H_
#define WK_LIB_WINDOW_H_

#include "properties.h"
#include "types.h"

#define WINDOW_MIN_WIDTH 80

typedef enum
{
    WK_STATUS_RUNNING,
    WK_STATUS_DAMAGED,
    WK_STATUS_EXIT_OK,
    WK_STATUS_EXIT_SOFTWARE,
} WkStatus;

int run(WkProperties* props);

#endif /* WK_LIB_WINDOW_H_ */
