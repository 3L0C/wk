#ifndef WK_WAYLAND_REGISTRY_H_
#define WK_WAYLAND_REGISTRY_H_

#include <stdbool.h>
#include <stdint.h>

/* common includes */
#include "common/menu.h"

typedef struct Wayland Wayland;

typedef uint8_t XkbModBit;
enum
{
    XKB_MOD_NONE  = 0,
    XKB_MOD_SHIFT = 1 << 0,
    XKB_MOD_CAPS  = 1 << 1,
    XKB_MOD_CTRL  = 1 << 2,
    XKB_MOD_ALT   = 1 << 3,
    XKB_MOD_MOD2  = 1 << 4,
    XKB_MOD_MOD3  = 1 << 5,
    XKB_MOD_LOGO  = 1 << 6,
    XKB_MOD_MOD5  = 1 << 7,
};

typedef uint8_t XkbModMask;
enum
{
    MASK_SHIFT,
    MASK_CAPS,
    MASK_CTRL,
    MASK_ALT,
    MASK_MOD2,
    MASK_MOD3,
    MASK_LOGO,
    MASK_MOD5,
    MASK_LAST,
};

void waylandRepeat(Wayland* wayland);
void waylandRegistryDestroy(Wayland* wayland);
bool waylandRegistryRegister(Wayland* wayland, Menu* props);

#endif /* WK_WAYLAND_REGISTRY_H_ */
