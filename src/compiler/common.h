#ifndef WK_COMPILER_COMMON_H_
#define WK_COMPILER_COMMON_H_

#include <stdbool.h>

#include "common/arena.h"
#include "common/vector.h"

bool compileKeys(Arena* arena, const char* keys, Vector* outKeys);

#endif /* WK_COMPILER_COMMON_H_ */
