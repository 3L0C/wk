#ifndef WK_COMMON_H_
#define WK_COMMON_H_

#include "lib/client.h"
#include "lib/window.h"

char* readFile(const char* path);
void parseArgs(Client* client, int* argc, char*** argv);
bool tryStdin(Client* client);

#endif /* WK_COMMON_H_ */
