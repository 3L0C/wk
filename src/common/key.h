#ifndef WK_COMMON_KEY_H_
#define WK_COMMON_KEY_H_

#include <stdbool.h>

/* common includes */
#include "modifier.h"
#include "special_key.h"
#include "string.h"

typedef struct
{
    String     repr;
    Modifier   mods;
    SpecialKey special;
} Key;

void keyCopy(const Key* from, Key* to);
void keyFree(Key* key);
void keyInit(Key* key);
bool keyIsEqual(const Key* a, const Key* b);

#endif /* WK_COMMON_KEY_H_ */
