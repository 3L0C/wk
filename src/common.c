#include <assert.h>
#include <bits/getopt_core.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/window.h"

#include "common.h"

static void
usage(void)
{
    fputs(
        "usage: wk [options]\n"
        "\n"
        "wk - Which-key via X11 and Wayland.\n"
        "\n"
        "Options:\n"
        "    -h, --help                 Display help message and exit.\n"
        "    -v, --version              Display version number and exit.\n"
        "    -D, --debug                Print debug information.\n"
        "    -t, --top                  Position window at top of screen.\n"
        "    -b, --bottom               Position window at bottom of screen.\n"
        "    -d, --delimiter STRING     Set delimiter to STRING.\n"
        "    -m, --max-cols NUM         Set maximum columns to NUM.\n"
        "    -k, --press KEY(s)         Press KEY(s) before dispalying window.\n"
        "    -p, --parse FILE           Parse FILE and transpile to valid 'chords.h' via stdout.\n"
        "    -c, --chords FILE          Use FILE for chords rather than those in 'chords.h'.\n"
        "    -s, --script STRING        Use STRING for chords rather than those in 'chords.h'.\n"
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

void
parseArgs(Client* client, int* argc, char*** argv)
{
    assert(client && argc && argv);

#define ARG(arg)        ((*arg)[(optind == 1 ? optind : optind - 1)])

    int opt = '\0';
    static struct option longOpts[] = {
        /*                  no argument                 */
        { "help",           no_argument,        0, 'h' },
        { "version",        no_argument,        0, 'v' },
        { "debug",          no_argument,        0, 'D' },
        { "top",            no_argument,        0, 't' },
        { "bottom",         no_argument,        0, 'b' },
        /*                  required argument           */
        { "delimiter",      required_argument,  0, 'd' },
        { "max-cols",       required_argument,  0, 'm' },
        { "press",          required_argument,  0, 'k' },
        { "parse",          required_argument,  0, 'p' },
        { "chords",         required_argument,  0, 'c' },
        { "script",         required_argument,  0, 's' },
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

        opt = getopt_long(*argc, *argv, ":hvDtbd:m:k:p:c:s:", longOpts, NULL);
        if (opt < 0) break;

        switch (opt)
        {
        /* no argument */
        case 'h': usage(); exit(EXIT_FAILURE);
        case 'v': puts(VERSION); exit(EXIT_SUCCESS);
        case 'D': client->debug = true; break;
        case 't': client->windowPosition = WK_WINDOW_TOP; break;
        case 'b': client->windowPosition = WK_WINDOW_BOTTOM; break;
        /* requires argument */
        case 'd': client->delimiter = optarg; break;
        case 'm': client->maxCols = atoi(optarg); break;
        case 0x090: client->windowWidth = atoi(optarg); break;
        case 0x091: client->windowHeight = atoi(optarg); break;
        case 0x092: client->borderWidth = atoi(optarg); break;
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
        case 'c': client->chords = optarg; break;
        case 's': client->script = optarg; break;
        /* Errors */
        case '?':
            usage();
            fprintf(stderr, "[ERROR] Unrecognized option: '%s'.\n", ARG(argv));
            exit(EXIT_FAILURE);
        case ':':
            usage();
            fprintf(stderr, "[ERROR] '%s' requires an argument but none given.\n", ARG(argv));
            exit(EXIT_FAILURE);
        default: usage(); exit(EXIT_FAILURE); break;
        }
    }

    *argc -= optind;
    *argv += optind;
#undef ARG
}
