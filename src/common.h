#ifndef WK_COMMON_H_
#define WK_COMMON_H_

#include "lib/client.h"
#include "lib/window.h"

void parseArgs(Client* client, int* argc, char*** argv);
WkStatus pressKeys(WkProperties* props, const char* keys);
char* readFile(const char* path);
bool tryStdin(Client* client);

#endif /* WK_COMMON_H_ */
