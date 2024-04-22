#ifndef WK_COMMON_COMMON_H_
#define WK_COMMON_COMMON_H_

#include <stdbool.h>

void errorMsg(const char* fmt, ...);
bool isUtf8ContByte(char byte);
bool isUtf8MultiByteStartByte(char byte);
bool isUtf8StartByte(char byte);
char* readFile(const char* filepath);
void warnMsg(const char* fmt, ...);

#endif /* WK_COMMON_COMMON_H_ */
