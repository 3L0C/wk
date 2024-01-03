#ifndef WK_LIB_WINDOW_H_
#define WK_LIB_WINDOW_H_

#include "common.h"

typedef enum
{
    WK_WINDOW_BOTTOM,
    WK_WINDOW_TOP
} WindowPosition;

typedef enum
{
    WK_FOREGROUND_COLOR,
    WK_BACKGROUND_COLOR,
    WK_BORDER_COLOR,
    WK_LAST_COLOR
} WkColor;

typedef struct
{
    char* hex;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} WkHexColor;

void render(void);

#endif /* WK_LIB_WINDOW_H_ */
