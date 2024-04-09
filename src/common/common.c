#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* local */
#include "common.h"
#include "memory.h"
#include "menu.h"

void
errorMsg(const char* fmt, ...)
{
    assert(fmt);
    static const int errorLen = strlen("[ERROR] ");

    int len = strlen(fmt) + 1; /* 1 = '\0' */
    char format[len + errorLen];
    memcpy(format, "[ERROR] ", errorLen);
    memcpy(format + errorLen, fmt, len);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, format, ap);
    va_end(ap);

    fputc((fmt[len - 2] == ':' ? ' ' : '\n'), stderr);
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
        "    -s, --script               Read script from stdin to use as key chords.\n"
        "    -m, --max-columns INT      Set maximum columns to INT.\n"
        "    -p, --press KEY(s)         Press KEY(s) before dispalying window.\n"
        "    -T, --transpile FILE       Transpile FILE to valid 'chords.h' syntax and print to stdout.\n"
        "    -c, --chords FILE          Use FILE for key chords rather than those in 'chords.h'.\n"
        "    -w, --width INT            Set window width to INT.\n"
        "    -g, --gap INT              Set window gap between top/bottom of screen to INT.\n"
        "                               Set to '-1' for a gap 1/10th the size of your screen height.\n"
        "    --border-width INT         Set border width to INT.\n"
        "    --border-radius NUM        Set border radius to NUM.\n"
        "    --wpadding INT             Set left and right padding around hint text to INT.\n"
        "    --hpadding INT             Set up and down padding around hint text to INT.\n"
        "    --fg COLOR                 Set window foreground to COLOR (e.g., '#F1CD39').\n"
        "    --bg COLOR                 Set window background to COLOR (e.g., '#F1CD39').\n"
        "    --bd COLOR                 Set window border to COLOR (e.g., '#F1CD39').\n"
        "    --shell STRING             Set shell to STRING (e.g., '/bin/sh').\n"
        "    --font STRING              Set font to STRING. Should be a valid Pango font description\n"
        "                               (e.g., 'monospace, M+ 1c, ..., 16').\n"
        "\n"
        "run `man 1 wk` for more info on each option.\n",
        stderr
    );
}

static bool
getInt(int* num)
{
    *num = atoi(optarg);
    if (*num != 0) return true;
    for (int i = 0; optarg[i] != '\0'; i++)
    {
        if (optarg[i] != '0') return false;
    }
    return true;
}

static bool
getNum(double* num)
{
    errno = 0;
    char* end;
    *num = strtod(optarg, &end);
    return (!(errno != 0 && *num == 0.0) && end != optarg);
}

void
parseArgs(WkMenu* menu, int* argc, char*** argv)
{
#define GET_ARG(arg)        ((*arg)[(optind == 1 ? optind : optind - 1)])

    assert(menu && argc && argv);

    int opt = '\0';
    static struct option longOpts[] = {
        /*                  no argument                 */
        { "help",           no_argument,        0, 'h' },
        { "version",        no_argument,        0, 'v' },
        { "debug",          no_argument,        0, 'd' },
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
        { "border-radius",  required_argument,  0, 0x091 },
        { "wpadding",       required_argument,  0, 0x092 },
        { "hpadding",       required_argument,  0, 0x093 },
        { "fg",             required_argument,  0, 0x094 },
        { "bg",             required_argument,  0, 0x095 },
        { "bd",             required_argument,  0, 0x096 },
        { "shell",          required_argument,  0, 0x097 },
        { "font",           required_argument,  0, 0x098 },
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
        case 'd': menu->debug = true; break;
        case 't': menu->position = WK_WIN_POS_TOP; break;
        case 'b': menu->position = WK_WIN_POS_BOTTOM; break;
        case 's': menu->client.tryScript = true; break;
        /* requires argument */
        case 'm':
        {
            int n = 0;
            if (!getInt(&n))
            {
                warnMsg("Could not convert '%s' into a integer.", optarg);
                warnMsg("Using default value of max-cols: %u.", menu->maxCols);
                break;
            }
            menu->maxCols = (uint32_t)n;
            break;
        }
        case 'p': menu->client.keys = optarg; break;
        case 'T': menu->client.transpile = optarg; break;
        case 'c': menu->client.keyChordsFile = optarg; break;
        case 'w':
        {
            int n = 0;
            if (!getInt(&n))
            {
                warnMsg("Could not convert '%s' into a integer.", optarg);
                warnMsg("Using default value for window-width: %u.", menu->windowWidth);
                break;
            }
            menu->windowWidth = n;
            break;
        }
        case 'g':
        {
            int n = 0;
            if (!getInt(&n))
            {
                warnMsg("Could not convert '%s' into a integer.", optarg);
                warnMsg("Using default value for window-gap: %u.", menu->windowGap);
                break;
            }
            menu->windowGap = n;
            break;
        }
        case 0x090:
        {
            int n = 0;
            if (!getInt(&n))
            {
                warnMsg("Could not convert '%s' into a integer.", optarg);
                warnMsg("Using default value for border-width: %u.", menu->borderWidth);
                break;
            }
            menu->borderWidth = (uint32_t)n;
            break;
        }
        case 0x091:
        {
            double n = 0.0;
            if (!getNum(&n))
            {
                warnMsg("Could not convert '%s' into a number.", optarg);
                warnMsg("Using default value for border-radius: %g", menu->borderRadius);
                break;
            }
            menu->borderRadius = n;
            break;
        }
        case 0x092:
        {
            int n = 0;
            if (!getInt(&n))
            {
                warnMsg("Could not convert '%s' into a integer.", optarg);
                warnMsg("Using default value for wpadding: %u.", menu->wpadding);
                break;
            }
            menu->wpadding = (uint32_t)n;
            break;
        }
        case 0x093:
        {
            int n = 0;
            if (!getInt(&n))
            {
                warnMsg("Could not convert '%s' into a integer.", optarg);
                warnMsg("Using default value for hpadding: %u.", menu->hpadding);
                break;
            }
            menu->hpadding = (uint32_t)n;
            break;
        }
        case 0x094: setMenuColor(menu, optarg, WK_COLOR_FOREGROUND); break;
        case 0x095: setMenuColor(menu, optarg, WK_COLOR_BACKGROUND); break;
        case 0x096: setMenuColor(menu, optarg, WK_COLOR_BORDER); break;
        case 0x097: menu->shell = optarg; break;
        case 0x098: menu->font = optarg; break;
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

char*
readFile(const char* path)
{
    assert(path);

    FILE* file = fopen(path, "rb");
    if (!file)
    {
        errorMsg("Could not open file '%s'.", path);
        goto fail;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = ALLOCATE(char, fileSize + 1);
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
fail:
    return NULL;
}

bool
tryStdin(WkMenu* menu)
{
    assert(menu);

    ssize_t n;
    size_t lineLength = 0;
    char* line = NULL;

    while ((n = getline(&line, &lineLength, stdin)) > 0)
    {
        /* addLineToScript(client, line, n); */
        appendToString(&menu->client.script, line, n);
    }
    free(line);

    return n == -1 && feof(stdin);
}

void
warnMsg(const char* fmt, ...)
{
    assert(fmt);

    static const int warnLen = strlen("[WARNING] ");

    int len = strlen(fmt) + 1; /* 1 = '\0' */
    char format[len + warnLen];
    memcpy(format, "[WARNING] ", warnLen);
    memcpy(format + warnLen, fmt, len);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, format, ap);
    va_end(ap);

    fputc((fmt[len - 2] == ':' ? ' ' : '\n'), stderr);
}
