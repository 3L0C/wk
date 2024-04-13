#ifndef WK_COMMON_COMMON_H_
#define WK_COMMON_COMMON_H_

#include <stdbool.h>

#include "menu.h"

void errorMsg(const char* fmt, ...);
void parseArgs(WkMenu* menu, int* argc, char*** argv);
char* readFile(const char* filepath);
bool tryStdin(WkMenu* menu);
void warnMsg(const char* fmt, ...);

#endif /* WK_COMMON_COMMON_H_ */
