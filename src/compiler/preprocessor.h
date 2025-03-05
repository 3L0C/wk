#ifndef WK_COMPILER_PREPROCESSOR_H_
#define WK_COMPILER_PREPROCESSOR_H_

#include <stdbool.h>

/* common includes */
#include "common/menu.h"

Array preprocessorRun(Menu* menu, Array* source, const char* sourcePath);

#endif /* WK_COMPILER_PREPROCESSOR_H_ */
