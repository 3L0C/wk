#ifndef WK_TRANSPILER_H_
#define WK_TRANSPILER_H_

#include "lib/common.h"

#include "common.h"
#include "compile.h"

bool transpileChords(Compiler* compiler, const char* source, const char* delimiter, bool debugFlag);

#endif /* WK_TRANSPILER_H_ */
