#ifndef WK_COMPILER_PREPROCESSOR_H_
#define WK_COMPILER_PREPROCESSOR_H_

#include <stdbool.h>

/* common includes */
#include "common/arena.h"
#include "common/menu.h"
#include "common/string.h"

String preprocessorRun(Menu* menu, String source, const char* sourcePath, Arena* arena);

#endif /* WK_COMPILER_PREPROCESSOR_H_ */
