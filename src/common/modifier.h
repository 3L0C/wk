#ifndef WK_COMMON_MODIFIER_H_
#define WK_COMMON_MODIFIER_H_

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t Modifier;
enum
{
    MOD_NONE  = 0,
    MOD_CTRL  = 1 << 0,
    MOD_META  = 1 << 1,
    MOD_HYPER = 1 << 2,
    MOD_SHIFT = 1 << 3
};

int      modifierCount(Modifier mod);
bool     modifierHasAnyActive(Modifier mod);
Modifier modifierInit(void);
bool     modifierIsActive(Modifier mod, Modifier test);

#endif /* WK_COMMON_MODIFIER_H_ */
