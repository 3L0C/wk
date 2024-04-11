#ifndef WK_COMPILER_PREPROCESSOR_H_
#define WK_COMPILER_PREPROCESSOR_H_

#include <stdbool.h>

/* common includes */
#include "common/menu.h"

char* runPreprocessor(WkMenu* menu, const char* source, const char* sourcePath);

#endif /* WK_COMPILER_PREPROCESSOR_H_ */
