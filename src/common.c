#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/client.h"
#include "lib/memory.h"
#include "lib/window.h"

#include "common.h"

char*
readFile(const char* path)
{
    assert(path);

    FILE* file = fopen(path, "rb");
    if (!file)
    {
        fprintf(stderr, "Could not open file '%s'.\n", path);
        goto error;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (!buffer)
    {
        fprintf(stderr, "Not enough memory to read '%s'.\n", path);
        goto alloc_error;
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize)
    {
        fprintf(stderr, "Could not read file '%s'.\n", path);
        goto read_error;
    }

    buffer[bytesRead] = '\0';
    fclose(file);

    return buffer;

read_error:
    free(buffer);
alloc_error:
    fclose(file);
error:
    return NULL;
}

static void
usage(void)
{
    fputs(
        "usage: wk [options]\n"
        "\n"
        "Options:\n"
        "    -h, --help                 Display help message and exit.\n"
        "    -v, --version              Display version number and exit.\n"
        "    -D, --debug                Print debug information.\n"
        "    -t, --top                  Position window at top of screen.\n"
        "    -b, --bottom               Position window at bottom of screen.\n"
        "    -s, --script               Read script from stdin to use as chords.\n"
        "    -d, --delimiter STRING     Set delimiter to STRING.\n"
        "    -m, --max-cols NUM         Set maximum columns to NUM.\n"
        "    -k, --press KEY(s)         Press KEY(s) before dispalying window.\n"
        "    -p, --parse FILE           Parse FILE and transpile to valid 'chords.h' via stdout.\n"
        "    -c, --chords FILE          Use FILE for chords rather than those in 'chords.h'.\n"
        "    --win-width NUM            Set window width to NUM\n"
        "    --win-height NUM           Set window height to NUM\n"
        "    --border-width NUM         Set border width to NUM\n"
        "    --fg COLOR                 Set window foreground to COLOR. i.e. '#AABBCC'\n"
        "    --bg COLOR                 Set window background to COLOR. i.e. '#AABBCC'\n"
        "    --bd COLOR                 Set window border to COLOR. i.e. '#AABBCC'\n"
        "    --shell STRING             Set shell to STRING, i.e. '/bin/sh'.\n"
        "    --font FONT_STRING         Add FONT_STRING to list of fonts.\n",
        stderr
    );
}

static bool
getNum(int* num)
{
    *num = atoi(optarg);
    if (*num != 0) return true;
    for (int i = 0; optarg[i] != '\0'; i++)
    {
        if (optarg[i] != '0') return false;
    }
    return true;
}

void
parseArgs(Client* client, int* argc, char*** argv)
{
#define GET_ARG(arg)        ((*arg)[(optind == 1 ? optind : optind - 1)])

    assert(client && argc && argv);

    int opt = '\0';
    static struct option longOpts[] = {
        /*                  no argument                 */
        { "help",           no_argument,        0, 'h' },
        { "version",        no_argument,        0, 'v' },
        { "debug",          no_argument,        0, 'D' },
        { "top",            no_argument,        0, 't' },
        { "bottom",         no_argument,        0, 'b' },
        { "script",         no_argument,        0, 's' },
        /*                  required argument           */
        { "delimiter",      required_argument,  0, 'd' },
        { "max-cols",       required_argument,  0, 'm' },
        { "press",          required_argument,  0, 'k' },
        { "parse",          required_argument,  0, 'p' },
        { "chords",         required_argument,  0, 'c' },
        { "win-width",      required_argument,  0, 0x090 },
        { "win-width",      required_argument,  0, 0x091 },
        { "border-width",   required_argument,  0, 0x092 },
        { "fg",             required_argument,  0, 0x093 },
        { "bg",             required_argument,  0, 0x094 },
        { "bd",             required_argument,  0, 0x095 },
        { "shell",          required_argument,  0, 0x096 },
        { "font",           required_argument,  0, 0x097 },
        { 0, 0, 0, 0 }
    };

    /* Don't let 'getopt' print errors. */
    opterr = 0;

    while (true)
    {

        opt = getopt_long(*argc, *argv, ":hvDtbsd:m:k:p:c:", longOpts, NULL);
        if (opt < 0) break;

        switch (opt)
        {
        /* no argument */
        case 'h': usage(); exit(EXIT_FAILURE);
        case 'v': puts(VERSION); exit(EXIT_SUCCESS);
        case 'D': client->debug = true; break;
        case 't': client->windowPosition = WK_WIN_POS_TOP; break;
        case 'b': client->windowPosition = WK_WIN_POS_BOTTOM; break;
        case 's': client->tryScript = true; break;
        /* requires argument */
        case 'd': client->delimiter = optarg; break;
        case 'm':
        {
            int n;
            if (!getNum(&n))
            {
                usage();
                fprintf(stderr, "[ERROR] Could not convert '%s' into a number.\n", optarg);
                exit(EXIT_FAILURE);
            }
            client->maxCols = (unsigned int)n;
            break;
        }
        case 0x090:
        {
            int n;
            if (!getNum(&n))
            {
                usage();
                fprintf(stderr, "[ERROR] Could not convert '%s' into a number.\n", optarg);
                exit(EXIT_FAILURE);
            }
            client->windowWidth = n;
            break;
        }
        case 0x091:
        {
            int n;
            if (!getNum(&n))
            {
                usage();
                fprintf(stderr, "[ERROR] Could not convert '%s' into a number.\n", optarg);
                exit(EXIT_FAILURE);
            }
            client->windowHeight = n;
            break;
        }
        case 0x092:
        {
            int n;
            if (!getNum(&n))
            {
                usage();
                fprintf(stderr, "[ERROR] Could not convert '%s' into a number.\n", optarg);
                exit(EXIT_FAILURE);
            }
            client->borderWidth = (unsigned int)n;
            break;
        }
        case 0x093: client->foreground = optarg; break;
        case 0x094: client->background = optarg; break;
        case 0x095: client->border = optarg; break;
        case 0x096: client->shell = optarg; break;
        case 0x097:
            if (client->fontCount < client->fontSize)
            {
                client->fonts[client->fontCount++] = optarg;
            }
            else
            {
                fprintf(
                    stderr,
                    "[WARNING] Cannot have more than %d fonts: '%s'.\n",
                    MAX_FONTS,
                    optarg
                );
            }
            break;
        case 'k': client->keys = optarg; break;
        case 'p': client->parse = optarg; break;
        case 'c': client->chordsFile = optarg; break;
        /* Errors */
        case '?':
            usage();
            fprintf(stderr, "[ERROR] Unrecognized option: '%s'.\n", GET_ARG(argv));
            exit(EXIT_FAILURE);
        case ':':
            usage();
            fprintf(stderr, "[ERROR] '%s' requires an argument but none given.\n", GET_ARG(argv));
            exit(EXIT_FAILURE);
        default: usage(); exit(EXIT_FAILURE); break;
        }
    }

    *argc -= optind;
    *argv += optind;

    if (*argc > 0)
    {
        fprintf(stderr, "[WARNING] Ignoring additional arguments: ");
        for (int i = 0; i < *argc; i++)
        {
            fprintf(stderr, "'%s'%c", (*argv)[i], (i + 1 == *argc ? '\n' : ' '));
        }
    }
#undef GET_ARG
}

static void
addLineToScript(Client* client, const char* line, const size_t n)
{
    while (client->scriptCount + n > client->scriptCapacity)
    {
        size_t oldCapacity = client->scriptCapacity;
        client->scriptCapacity = GROW_CAPACITY(oldCapacity);
        client->script = GROW_ARRAY(
            char, client->script, oldCapacity, client->scriptCapacity
        );
    }

    /* careful not to copy the '\0' byte from getline() */
    memcpy(client->script + (client->scriptCount ? client->scriptCount - 1 : 0), line, n);
    client->scriptCount += n;
}

bool
tryStdin(Client* client)
{
    assert(client);

    ssize_t n;
    size_t lineLength = 0;
    char* line = NULL;

    while ((n = getline(&line, &lineLength, stdin)) > 0)
    {
        addLineToScript(client, line, n + 1);
    }
    free(line);

    return n == -1 && feof(stdin);
}
