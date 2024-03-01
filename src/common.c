#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/client.h"
#include "lib/common.h"
#include "lib/debug.h"
#include "lib/memory.h"
#include "lib/properties.h"
#include "lib/types.h"
#include "lib/util.h"
#include "lib/window.h"

#include "common.h"
#include "scanner.h"

bool
statusIsError(WkStatus status)
{
    return status == WK_STATUS_EXIT_SOFTWARE;
}

static void
usage(void)
{
    fputs(
        "usage: wk [options]\n"
        "\n"
        "options:\n"
        "    -h, --help                 Display help message and exit.\n"
        "    -v, --version              Display version number and exit.\n"
        "    -d, --debug                Print debug information.\n"
        "    -t, --top                  Position window at top of screen.\n"
        "    -b, --bottom               Position window at bottom of screen.\n"
        "    -s, --script               Read script from stdin to use as chords.\n"
        "    -m, --max-columns NUM      Set maximum columns to NUM.\n"
        "    -p, --press KEY(s)         Press KEY(s) before dispalying window.\n"
        "    -T, --transpile FILE       Transpile FILE to valid 'chords.h' syntax and print to stdout.\n"
        "    -c, --chords FILE          Use FILE for chords rather than those in 'chords.h'.\n"
        "    -w, --width NUM            Set window width to NUM.\n"
        "    -g, --gap NUM              Set window gap between top/bottom of screen.\n"
        "                               Set to '-1' for a gap 1/10th the size of your screen height.\n"
        "    --border-width NUM         Set border width to NUM.\n"
        "    --wpadding NUM             Set left and right padding around hint text to NUM.\n"
        "    --hpadding NUM             Set up and down padding around hint text to NUM.\n"
        "    --fg COLOR                 Set window foreground to COLOR (e.g., '#F1CD39').\n"
        "    --bg COLOR                 Set window background to COLOR (e.g., '#F1CD39').\n"
        "    --bd COLOR                 Set window border to COLOR (e.g., '#F1CD39').\n"
        "    --shell STRING             Set shell to STRING (e.g., '/bin/sh').\n"
        "    --font STRING              Set font to STRING. Should be a valid Pango font description\n"
        "                               (e.g., 'monospace, M+ 1c, ..., 16').\n",
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
        { "max-columns",    required_argument,  0, 'm' },
        { "press",          required_argument,  0, 'p' },
        { "transpile",      required_argument,  0, 'T' },
        { "chords",         required_argument,  0, 'c' },
        { "width",          required_argument,  0, 'w' },
        { "gap",            required_argument,  0, 'g' },
        { "border-width",   required_argument,  0, 0x090 },
        { "wpadding",       required_argument,  0, 0x091 },
        { "hpadding",       required_argument,  0, 0x092 },
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

        opt = getopt_long(*argc, *argv, ":hvdtbsm:p:T:c:w:g:", longOpts, NULL);
        if (opt < 0) break;

        switch (opt)
        {
        /* no argument */
        case 'h': usage(); exit(EXIT_FAILURE);
        case 'v': puts("wk v"VERSION); exit(EXIT_SUCCESS);
        case 'd': client->debug = true; break;
        case 't': client->windowPosition = WK_WIN_POS_TOP; break;
        case 'b': client->windowPosition = WK_WIN_POS_BOTTOM; break;
        case 's': client->tryScript = true; break;
        /* requires argument */
        case 'm':
        {
            int n;
            if (!getNum(&n))
            {
                usage();
                errorMsg("Could not convert '%s' into a number.", optarg);
                exit(EXIT_FAILURE);
            }
            client->maxCols = (unsigned int)n;
            break;
        }
        case 'p': client->keys = optarg; break;
        case 'T': client->transpile = optarg; break;
        case 'c': client->chordsFile = optarg; break;
        case 'w':
        {
            int n;
            if (!getNum(&n))
            {
                usage();
                errorMsg("Could not convert '%s' into a number.", optarg);
                exit(EXIT_FAILURE);
            }
            client->windowWidth = n;
            break;
        }
        case 'g':
        {
            int n;
            if (!getNum(&n))
            {
                usage();
                errorMsg("Could not convert '%s' into a number.", optarg);
                exit(EXIT_FAILURE);
            }
            client->windowGap = n;
            break;
        }
        case 0x090:
        {
            int n;
            if (!getNum(&n))
            {
                usage();
                errorMsg("Could not convert '%s' into a number.", optarg);
                exit(EXIT_FAILURE);
            }
            client->borderWidth = (unsigned int)n;
            break;
        }
        case 0x091:
        {
            int n;
            if (!getNum(&n))
            {
                usage();
                errorMsg("Could not convert '%s' into a number.", optarg);
                exit(EXIT_FAILURE);
            }
            client->wpadding = (unsigned int)n;
            break;
        }
        case 0x092:
        {
            int n;
            if (!getNum(&n))
            {
                usage();
                errorMsg("Could not convert '%s' into a number.", optarg);
                exit(EXIT_FAILURE);
            }
            client->hpadding = (unsigned int)n;
            break;
        }
        case 0x093: client->foreground = optarg; break;
        case 0x094: client->background = optarg; break;
        case 0x095: client->border = optarg; break;
        case 0x096: client->shell = optarg; break;
        case 0x097: client->font = optarg; break;
        /* Errors */
        case '?':
            usage();
            errorMsg("Unrecognized option: '%s'.", GET_ARG(argv));
            exit(EXIT_FAILURE);
        case ':':
            usage();
            errorMsg("'%s' requires an argument but none given.", GET_ARG(argv));
            exit(EXIT_FAILURE);
        default: usage(); exit(EXIT_FAILURE); break;
        }
    }

    *argc -= optind;
    *argv += optind;

    if (*argc > 0)
    {
        warnMsg("Ignoring additional arguments:");
        for (int i = 0; i < *argc; i++)
        {
            fprintf(stderr, "'%s'%c", (*argv)[i], (i + 1 == *argc ? '\n' : ' '));
        }
    }
#undef GET_ARG
}

static bool
addMod(Key* key, TokenType type)
{
    switch (type)
    {
    case TOKEN_MOD_CTRL: key->mods.ctrl = true; break;
    case TOKEN_MOD_ALT: key->mods.alt = true; break;
    case TOKEN_MOD_HYPER: key->mods.hyper = true; break;
    case TOKEN_MOD_SHIFT: key->mods.shift = true; break;
    default: return false;
    }

    return true;
}

static bool
addSpecial(Key* key, TokenType type)
{
    key->key = NULL;

    switch (type)
    {
    case TOKEN_SPECIAL_LEFT: key->special = WK_SPECIAL_LEFT; break;
    case TOKEN_SPECIAL_RIGHT: key->special = WK_SPECIAL_RIGHT; break;
    case TOKEN_SPECIAL_UP: key->special = WK_SPECIAL_UP; break;
    case TOKEN_SPECIAL_DOWN: key->special = WK_SPECIAL_DOWN; break;
    case TOKEN_SPECIAL_TAB: key->special = WK_SPECIAL_TAB; break;
    case TOKEN_SPECIAL_SPACE: key->special = WK_SPECIAL_SPACE; break;
    case TOKEN_SPECIAL_RETURN: key->special = WK_SPECIAL_RETURN; break;
    case TOKEN_SPECIAL_DELETE: key->special = WK_SPECIAL_DELETE; break;
    case TOKEN_SPECIAL_ESCAPE: key->special = WK_SPECIAL_ESCAPE; break;
    case TOKEN_SPECIAL_HOME: key->special = WK_SPECIAL_HOME; break;
    case TOKEN_SPECIAL_PAGE_UP: key->special = WK_SPECIAL_PAGE_UP; break;
    case TOKEN_SPECIAL_PAGE_DOWN: key->special = WK_SPECIAL_PAGE_DOWN; break;
    case TOKEN_SPECIAL_END: key->special = WK_SPECIAL_END; break;
    case TOKEN_SPECIAL_BEGIN: key->special = WK_SPECIAL_BEGIN; break;
    default: return false;
    }

    return true;
}

static WkStatus
pressKey(WkProperties* props, Scanner* scanner)
{
    assert(props && scanner);

    static const size_t bufmax = 32;
    char buffer[bufmax];
    memset(buffer, 0, 32);
    Key key = {0};
    Token token = scanToken(scanner);

    while (addMod(&key, token.type))
    {
        token = scanToken(scanner);
    }

    if (token.type == TOKEN_KEY)
    {
        key.special = WK_SPECIAL_NONE;
        if (token.length > bufmax)
        {
            errorMsg("Key is longer than max size of %zu: %04zu", bufmax, token.length);
            return WK_STATUS_EXIT_SOFTWARE;
        }
        memcpy(buffer, token.start, token.length);
        buffer[token.length] = '\0';
        key.key = buffer;
        key.len = token.length;
    }
    else if (!addSpecial(&key, token.type))
    {
        errorMsg(
            "Key does not appear to be a regular key or a special key: '%.*s'.",
            (int)token.length, token.start
        );
        return WK_STATUS_EXIT_SOFTWARE;
    }

    if (props->debug)
    {
        debugMsg(props->debug, "Trying to press key: '%.*s'.", (int)token.length, token.start);
        debugKey(&key);
    }

    WkStatus status = handleKeypress(props, &key);

    if (status == WK_STATUS_EXIT_SOFTWARE)
    {
        errorMsg("Could not press key: '%.*s'.", (int)token.length, token.start);
        return status;
    }

    if (status == WK_STATUS_EXIT_OK)
    {
        token = scanToken(scanner);
        if (token.type == TOKEN_EOF) return status;
        return WK_STATUS_EXIT_SOFTWARE;
    }

    return status;
}

static bool
statusIsRunning(WkStatus status)
{
    return status == WK_STATUS_RUNNING || status == WK_STATUS_DAMAGED;
}

WkStatus
pressKeys(WkProperties* props, const char* keys)
{
    assert(props && keys);

    Scanner scanner;
    initScanner(&scanner, keys);
    WkStatus status = pressKey(props, &scanner);

    while (*scanner.current != '\0' && statusIsRunning(status))
    {
        status = pressKey(props, &scanner);
    }

    if (status == WK_STATUS_EXIT_OK && *scanner.current != '\0')
    {
        errorMsg(
            "Reached end of chords but not end of keys: '%s'.",
            (scanner.current == scanner.start) ? scanner.start : scanner.current - 1
        );
        return status;
    }

    return status;
}

char*
readFile(const char* path)
{
    assert(path);

    FILE* file = fopen(path, "rb");
    if (!file)
    {
        errorMsg("Could not open file '%s'.", path);
        goto error;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (!buffer)
    {
        errorMsg("Not enough memory to read '%s'.", path);
        goto alloc_error;
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize)
    {
        errorMsg("Could not read file '%s'.", path);
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
    memcpy(&client->script[client->scriptCount], line, n);
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
        addLineToScript(client, line, n);
    }
    free(line);

    return n == -1 && feof(stdin);
}
