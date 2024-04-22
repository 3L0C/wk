#ifndef WK_RUNTIME_COMMON_H_
#define WK_RUNTIME_COMMON_H_

#include <stdbool.h>
#include <stddef.h>

/* common includes */
#include "common/key_chord.h"

typedef SpecialKey (*GetSpecialKeyFp)(void*);

bool isNormalKey(Key* key, char* buffer, size_t len);
bool isSpecialKey(Key* key, void* keysym, GetSpecialKeyFp fp);

#endif /* WK_RUNTIME_COMMON_H_ */
