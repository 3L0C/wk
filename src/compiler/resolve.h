#ifndef WK_COMPILER_RESOLVE_H_
#define WK_COMPILER_RESOLVE_H_

#include <stdbool.h>

/* common includes */
#include "common/menu.h"
#include "common/vector.h"

/* local includes */
#include "scanner.h"

bool resolve(Vector* chords, Menu* menu, Scanner* scanner);

#endif /* WK_COMPILER_RESOLVE_H_ */
