#ifndef WK_WAYLAND_REGISTRY_H_
#define WK_WAYLAND_REGISTRY_H_

#include <stdbool.h>

#include "lib/menu.h"

typedef struct Wayland Wayland;

typedef enum
{
    MOD_SHIFT = 1<<0,
    MOD_CAPS = 1<<1,
    MOD_CTRL = 1<<2,
    MOD_ALT = 1<<3,
    MOD_MOD2 = 1<<4,
    MOD_MOD3 = 1<<5,
    MOD_LOGO = 1<<6,
    MOD_MOD5 = 1<<7,
} XkbModBit;

typedef enum
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
} XkbModMask;

extern const char* WK_XKB_MASK_NAMES[MASK_LAST];
extern const XkbModBit WK_XKB_MODS[MASK_LAST];

void waylandRepeat(Wayland* wayland);
void waylandRegistryDestroy(Wayland* wayland);
bool waylandRegistryRegister(Wayland* wayland, WkMenu* props);

#endif /* WK_WAYLAND_REGISTRY_H_ */
