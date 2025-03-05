#ifndef WK_COMMON_COMMON_H_
#define WK_COMMON_COMMON_H_

#include <stdbool.h>

/* common includes */
#include "common/array.h"

void errorMsg(const char* fmt, ...);
char getSeparator(int* count, char a, char b);
bool isUtf8ContByte(char byte);
bool isUtf8MultiByteStartByte(char byte);
bool isUtf8StartByte(char byte);
Array readFile(const char* filepath);
void warnMsg(const char* fmt, ...);

#endif /* WK_COMMON_COMMON_H_ */
