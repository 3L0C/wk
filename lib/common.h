#ifndef WK_LIB_COMMON_H_
#define WK_LIB_COMMON_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*CleanupFP)(void* xp);

void errorMsg(const char* fmt, ...);
void warnMsg(const char* fmt, ...);

#endif /* WK_LIB_COMMON_H_ */
