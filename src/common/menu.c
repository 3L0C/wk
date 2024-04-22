#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>

#ifdef WK_X11_BACKEND
#include "runtime/x11/window.h"
#endif

#ifdef WK_WAYLAND_BACKEND
#include "runtime/wayland/wayland.h"
#endif

/* config include */
#include "config/config.h"

/* local includes */
#include "common.h"
#include "debug.h"
#include "menu.h"
#include "key_chord.h"

void
countMenuKeyChords(Menu* menu)
{
    assert(menu);

    menu->keyChordCount = countKeyChords(menu->keyChords);
}

int
displayMenu(Menu* menu)
{
    assert(menu);

#ifdef WK_WAYLAND_BACKEND
    if (getenv("WAYLAND_DISPLAY") || getenv("WAYLAND_SOCKET"))
    {
        debugMsg(menu->debug, "Running on wayland.");
        return runWayland(menu);
    }
#endif
#ifdef WK_X11_BACKEND
    debugMsg(menu->debug, "Running on x11.");
    return runX11(menu);
#endif
    errorMsg("Can only run under X11 and/or Wayland.");
    return EX_SOFTWARE;
}

static MenuStatus
handlePrefix(Menu* menu, const KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    debugMsg(menu->debug, "Found prefix.");

    menu->keyChords = keyChord->keyChords;
    countMenuKeyChords(menu);
    return MENU_STATUS_DAMAGED;
}

static void
handleCommand(Menu* menu, const KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    if (keyChord->flags.write)
    {
        printf("%s\n", keyChord->command);
        return;
    }
    spawn(menu, keyChord->command, keyChord->flags.syncCommand);
}

static MenuStatus
handleCommands(Menu* menu, const KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    /* no command */
    if (!keyChord->command) return MENU_STATUS_EXIT_OK;

    if (keyChord->before) spawn(menu, keyChord->before, keyChord->flags.syncBefore);
    handleCommand(menu, keyChord);
    if (keyChord->after) spawn(menu, keyChord->after, keyChord->flags.syncAfter);
    return keyChord->flags.keep ? MENU_STATUS_RUNNING : MENU_STATUS_EXIT_OK;
}

static MenuStatus
pressKey(Menu* menu, const KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    if (keyChord->keyChords) return handlePrefix(menu, keyChord);
    return handleCommands(menu, keyChord);
}

MenuStatus
handleKeypress(Menu* menu, const Key* key)
{
    assert(menu), assert(key);

    uint32_t len = menu->keyChordCount;
    const KeyChord* keyChords = menu->keyChords;

    for (uint32_t i = 0; i < len; i++)
    {
        if (keysAreEqual(&keyChords[i].key, key))
        {
            if (menu->debug)
            {
                debugMsg(menu->debug, "Found match: '%s'.\n", keyChords[i].key);
                disassembleKeyChordWithHeader(&keyChords[i], 0);
                disassembleKey(key);
            }
            return pressKey(menu, &keyChords[i]);
        }
    }

    if (menu->debug)
    {
        debugMsg(menu->debug, "Did not find a match for keypress.\n");
        disassembleKey(key);
    }

    return MENU_STATUS_EXIT_SOFTWARE;
}

static bool
initColor(MenuHexColor* hexColor, const char* color)
{
    assert(hexColor), assert(color);

    unsigned int r, g, b, a = 255;
    int count = sscanf(color, "#%2x%2x%2x%2x", &r, &g, &b, &a);

    if (count != 3 && count != 4) return false;

    hexColor->hex = color;
    hexColor->r = r;
    hexColor->g = g;
    hexColor->b = b;
    hexColor->a = a;

    return true;
}

static void
initColors(MenuHexColor* hexColors)
{
    assert(hexColors);

    static const char* defaultColors[MENU_COLOR_LAST] = {
        "#DCD7BA", "#181616", "#7FB4CA"
    };

    const char* colors[MENU_COLOR_LAST] = {
        foreground, background, border
    };
    for (int i = 0; i < MENU_COLOR_LAST; i++)
    {
        if (!initColor(&hexColors[i], colors[i]))
        {
            char* colorType;
            warnMsg("Invalid color string '%s':", colors[i]);
            switch (i)
            {
            case MENU_COLOR_FOREGROUND: colorType = "foreground"; break;
            case MENU_COLOR_BACKGROUND: colorType = "background"; break;
            case MENU_COLOR_BORDER: colorType = "border"; break;
            default: colorType = "UNKNOWN"; break;
            }
            fprintf(stderr, "setting %s to '%s'.\n", colorType, defaultColors[i]);
            initColor(&hexColors[i], defaultColors[i]);
        }
    }
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
        "    -S, --sort                 Sort key chords read from --key-chords, or --script.\n"
        "    -m, --max-columns INT      Set maximum columns to INT.\n"
        "    -p, --press KEY(s)         Press KEY(s) before dispalying window.\n"
        "    -T, --transpile FILE       Transpile FILE to valid 'key_chords.h' syntax and print to stdout.\n"
        "    -k, --key-chords FILE      Use FILE for key chords rather than those precompiled.\n"
        "    -w, --window-width INT     Set window width to INT.\n"
        "    -g, --window-gap INT       Set window gap between top/bottom of screen to INT.\n"
        "                               Set to '-1' for a gap equal to 1/10th of the screen height.\n"
        "    --border-width INT         Set border width to INT.\n"
        "    --border-radius NUM        Set border radius to NUM.\n"
        "    --wpadding INT             Set left and right padding around hint text to INT.\n"
        "    --hpadding INT             Set top and bottom padding around hint text to INT.\n"
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
    assert(num);

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
    assert(num);

    errno = 0;
    char* end;
    *num = strtod(optarg, &end);
    return (!(errno != 0 && *num == 0.0) && end != optarg);
}

void
parseArgs(Menu* menu, int* argc, char*** argv)
{
#define GET_ARG(arg)        ((*arg)[(optind == 1 ? optind : optind - 1)])

    assert(menu), assert(argc), assert(argv);

    int opt = '\0';
    static struct option longOpts[] = {
        /*                  no argument                 */
        { "help",           no_argument,        0, 'h' },
        { "version",        no_argument,        0, 'v' },
        { "debug",          no_argument,        0, 'd' },
        { "top",            no_argument,        0, 't' },
        { "bottom",         no_argument,        0, 'b' },
        { "script",         no_argument,        0, 's' },
        { "sort",           no_argument,        0, 'S' },
        /*                  required argument           */
        { "max-columns",    required_argument,  0, 'm' },
        { "press",          required_argument,  0, 'p' },
        { "transpile",      required_argument,  0, 'T' },
        { "key-chords",     required_argument,  0, 'k' },
        { "window-width",   required_argument,  0, 'w' },
        { "window-gap",     required_argument,  0, 'g' },
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

        opt = getopt_long(*argc, *argv, ":hvdtbsSm:p:T:c:w:g:", longOpts, NULL);
        if (opt < 0) break;

        switch (opt)
        {
        /* no argument */
        case 'h': usage(); exit(EXIT_FAILURE);
        case 'v': puts("wk v"VERSION); exit(EXIT_SUCCESS);
        case 'd': menu->debug = true; break;
        case 't': menu->position = MENU_WIN_POS_TOP; break;
        case 'b': menu->position = MENU_WIN_POS_BOTTOM; break;
        case 's': menu->client.tryScript = true; break;
        case 'S': menu->sort = true; break;
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
        case 'k': menu->client.wksFile = optarg; break;
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
        case 0x094: setMenuColor(menu, optarg, MENU_COLOR_FOREGROUND); break;
        case 0x095: setMenuColor(menu, optarg, MENU_COLOR_BACKGROUND); break;
        case 0x096: setMenuColor(menu, optarg, MENU_COLOR_BORDER); break;
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

void
initMenu(Menu* menu, KeyChord* keyChords)
{
    assert(menu);

    menu->delimiter = delimiter;
    menu->maxCols = maxCols;
    menu->windowWidth = windowWidth;
    menu->windowGap = windowGap;
    menu->wpadding = widthPadding;
    menu->hpadding = heightPadding;
    menu->cellHeight = 0;
    menu->rows = 0;
    menu->cols = 0;
    menu->width = 0;
    menu->height = 0;
    menu->position = (windowPosition ? MENU_WIN_POS_TOP : MENU_WIN_POS_BOTTOM);
    menu->borderWidth = borderWidth;
    menu->borderRadius = borderRadius;
    initColors(menu->colors);
    menu->shell = shell;
    menu->font = font;
    menu->keyChords = keyChords;
    menu->keyChordsHead = NULL;
    menu->keyChordCount = 0;
    menu->debug = false;
    menu->sort = false;
    menu->dirty = true;
    menu->client.keys = NULL;
    menu->client.transpile = NULL;
    menu->client.wksFile = NULL;
    menu->client.tryScript = false;
    initString(&menu->client.script);
    menu->garbage.shell = NULL;
    menu->garbage.font = NULL;
    menu->garbage.foregroundColor = NULL;
    menu->garbage.backgroundColor = NULL;
    menu->garbage.borderColor = NULL;
    menu->cleanupfp = NULL;
    menu->xp = NULL;
}

void
setMenuColor(Menu* menu, const char* color, MenuColor colorType)
{
    assert(menu), assert(colorType < MENU_COLOR_LAST), assert(!(colorType < 0));

    if (!initColor(&menu->colors[colorType], color)) warnMsg("Invalid color string: '%s'.", color);
}

static MenuStatus
spawnSync(const char* shell, const char* cmd)
{
    assert(shell), assert(cmd);

    setsid();
    char* exec[] = { strdup(shell), "-c", strdup(cmd), NULL };
    if (!exec[0])
    {
        errorMsg("Could not duplicate shell string: '%s'.", shell);
        goto fail;
    }
    if (!exec[2])
    {
        errorMsg("Could not duplicate command string: '%s'.", cmd);
        goto fail;
    }
    execvp(exec[0], exec);
    errorMsg("Failed to spawn command: '%s -c %s'.", shell, cmd);
fail:
    free(exec[0]);
    free(exec[2]);
    return MENU_STATUS_EXIT_SOFTWARE;
}

static MenuStatus
spawnAsync(const char* shell, const char* cmd)
{
    assert(shell), assert(cmd);

    if (fork() == 0)
    {
        spawnSync(shell, cmd);
    }
    return MENU_STATUS_EXIT_OK;
}

MenuStatus
spawn(const Menu* menu, const char* cmd, bool sync)
{
    assert(menu), assert(cmd);

    pid_t child = fork();

    if (child == -1)
    {
        errorMsg("Could not fork process:");
        perror(NULL);
        return MENU_STATUS_EXIT_SOFTWARE;
    }

    if (child == 0)
    {
        if (menu->xp && menu->cleanupfp) menu->cleanupfp(menu->xp);
        if (sync)
        {
            spawnSync(menu->shell, cmd);
        }
        else
        {
            spawnAsync(menu->shell, cmd);
        }
        exit(EX_OK);
    }

    if (sync)
    {
        int status;
        if (waitpid(child, &status, 0) == -1)
        {
            errorMsg("Could not wait for child process:");
            perror(NULL);
            return MENU_STATUS_EXIT_SOFTWARE;
        }

        return WIFEXITED(status) ? MENU_STATUS_EXIT_OK : MENU_STATUS_EXIT_SOFTWARE;
    }

    wait(NULL);
    return MENU_STATUS_EXIT_OK;
}

bool
statusIsError(MenuStatus status)
{
    return status == MENU_STATUS_EXIT_SOFTWARE;
}

bool
statusIsRunning(MenuStatus status)
{
    return status == MENU_STATUS_RUNNING || status == MENU_STATUS_DAMAGED;
}
bool
tryStdin(Menu* menu)
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
