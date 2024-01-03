#include <stdio.h>

#include "chords.h"
#include "client.h"
#include "common.h"

static Client client;

int
main(int argc, char** argv)
{
    initClient(&client);
    parseArgs(&client, &argc, &argv);
    return 0;
}
