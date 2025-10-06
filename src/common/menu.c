#include "arena.h"
#include "array.h"
#include <assert.h>
#include <bits/getopt_core.h>
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
#include <time.h>
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
#include "key_chord.h"
#include "menu.h"
#include "string.h"

int
menuDisplay(Menu* menu)
{
    assert(menu);

    menuResetTimer(menu);

#ifdef WK_WAYLAND_BACKEND
    if (getenv("WAYLAND_DISPLAY") || getenv("WAYLAND_SOCKET"))
    {
        debugMsg(menu->debug, "Running on wayland.");
        return waylandRun(menu);
    }
#endif
#ifdef WK_X11_BACKEND
    debugMsg(menu->debug, "Running on x11.");
    menu->uwsmWrapper = false;
    return x11Run(menu);
#endif
    errorMsg("Can only run under X11 and/or Wayland.");
    return EX_SOFTWARE;
}

void
menuFree(Menu* menu)
{
    assert(menu);

    keyChordArrayFree(&menu->compiledKeyChords);
    arenaFree(&menu->arena);
}

static MenuStatus
menuHandlePrefix(Menu* menu, KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    debugMsg(menu->debug, "Found prefix.");

    menu->keyChords = &keyChord->keyChords;
    return MENU_STATUS_DAMAGED;
}

static void
menuHandleCommand(Menu* menu, KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    if (keyChord->flags & FLAG_WRITE)
    {
        char buffer[keyChord->command.length + 1];
        stringWriteToBuffer(&keyChord->command, buffer);
        printf("%s\n", buffer);
        return;
    }

    menuSpawn(menu, &keyChord->command, chordFlagIsActive(keyChord->flags, FLAG_SYNC_COMMAND));
}

static MenuStatus
menuHandleCommands(Menu* menu, KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    /* TODO handle before/after write rather than always spawn */
    menuSpawn(menu, &keyChord->before, chordFlagIsActive(keyChord->flags, FLAG_SYNC_BEFORE));
    menuHandleCommand(menu, keyChord);
    menuSpawn(menu, &keyChord->after, chordFlagIsActive(keyChord->flags, FLAG_SYNC_AFTER));

    return chordFlagIsActive(keyChord->flags, FLAG_KEEP) ? MENU_STATUS_RUNNING : MENU_STATUS_EXIT_OK;
}

static MenuStatus
menuPressKey(Menu* menu, KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    if (!arrayIsEmpty(&keyChord->keyChords)) return menuHandlePrefix(menu, keyChord);
    return menuHandleCommands(menu, keyChord);
}

MenuStatus
menuHandleKeypress(Menu* menu, const Key* key)
{
    assert(menu), assert(key);

    forEach(menu->keyChords, KeyChord, keyChord)
    {
        if (keyIsEqual(&keyChord->key, key))
        {
            if (menu->debug)
            {
                char buffer[keyChord->key.repr.length + 1];
                stringWriteToBuffer(&keyChord->key.repr, buffer);
                debugMsg(menu->debug, "Found match: '%s'.", buffer);
                disassembleKeyChordWithHeader(keyChord, 0);
                disassembleKey(key);
            }
            menu->dirty = true;
            return menuPressKey(menu, keyChord);
        }
    }

    if (menu->debug)
    {
        debugMsg(menu->debug, "Did not find a match for keypress.");
        disassembleKey(key);
    }

    return MENU_STATUS_EXIT_SOFTWARE;
}

static bool
menuHexColorInitColor(MenuHexColor* hexColor, const char* color)
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
menuHexColorInitColors(MenuHexColor* hexColors)
{
    assert(hexColors);

    static const char* defaultColors[MENU_COLOR_LAST] = {
        "#DCD7BA", /* Key color */
        "#525259", /* Delimiter color */
        "#AF9FC9", /* Prefix color */
        "#DCD7BA", /* Chord color */
        "#181616", /* Background */
        "#7FB4CA"  /* Border */
    };

    const char* colors[MENU_COLOR_LAST] = {
        foreground[FOREGROUND_COLOR_KEY],
        foreground[FOREGROUND_COLOR_DELIMITER],
        foreground[FOREGROUND_COLOR_PREFIX],
        foreground[FOREGROUND_COLOR_CHORD],
        background,
        border
    };
    for (int i = 0; i < MENU_COLOR_LAST; i++)
    {
        if (!menuHexColorInitColor(&hexColors[i], colors[i]))
        {
            char* colorType;
            warnMsg("Invalid color string '%s':", colors[i]);
            switch (i)
            {
            case MENU_COLOR_KEY: colorType = "key"; break;
            case MENU_COLOR_DELIMITER: colorType = "delimiter"; break;
            case MENU_COLOR_PREFIX: colorType = "prefix"; break;
            case MENU_COLOR_CHORD: colorType = "chord"; break;
            case MENU_COLOR_BACKGROUND: colorType = "background"; break;
            case MENU_COLOR_BORDER: colorType = "border"; break;
            default: colorType = "UNKNOWN"; break;
            }
            fprintf(stderr, "setting %s to '%s'.\n", colorType, defaultColors[i]);
            menuHexColorInitColor(&hexColors[i], defaultColors[i]);
        }
    }
}

void
menuInit(Menu* menu, Array* keyChords)
{
    assert(menu);

    menu->delimiter = delimiter;
    menu->shell = shell;
    menu->font = font;
    menu->implicitArrayKeys = implicitArrayKeys;
    menu->borderRadius = borderRadius;
    menuHexColorInitColors(menu->colors);
    menu->client.keys = NULL;
    menu->client.transpile = NULL;
    menu->client.wksFile = NULL;
    menu->client.tryScript = false;
    menu->client.script = ARRAY_INIT(char);
    clock_gettime(CLOCK_MONOTONIC, &menu->timer);
    menu->compiledKeyChords = ARRAY_INIT(KeyChord);
    menu->builtinKeyChords = keyChords;
    menu->keyChords = keyChords;
    menu->keyChordsHead = keyChords;
    menu->cleanupfp = NULL;
    menu->xp = NULL;
    arenaInit(&menu->arena);

    menu->maxCols = maxCols;
    menu->menuWidth = menuWidth;
    menu->menuGap = menuGap;
    menu->wpadding = widthPadding;
    menu->hpadding = heightPadding;
    menu->cellHeight = 0;
    menu->rows = 0;
    menu->cols = 0;
    menu->width = 0;
    menu->height = 0;
    menu->borderWidth = borderWidth;
    menu->delay = delay;

    menu->position = (menuPosition ? MENU_POS_TOP : MENU_POS_BOTTOM);
    menu->debug = false;
    menu->sort = false;
    menu->dirty = true;
    menu->uwsmWrapper = uwsmWrapper;
}

bool
menuIsDelayed(Menu* menu)
{
    assert(menu);
    if (!menu->delay) return false;

    static struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    long elapsedTime = (
        ((now.tv_sec - menu->timer.tv_sec) * 1000) +
        ((now.tv_nsec - menu->timer.tv_nsec) / 1000000)
    );

    bool result = elapsedTime < menu->delay;
    if (!result) menu->delay = 0;

    return result;
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
        "    -u, --uwsm                 Wrap commands with `uwsm app -- cmd [args ...]` (Wayland).\n"
        "    -D, --delay INT            Delay the popup menu by INT milliseconds from\n"
        "                               startup/last keypress (default 1000 ms).\n"
        "    -t, --top                  Position menu at top of screen.\n"
        "    -b, --bottom               Position menu at bottom of screen.\n"
        "    -s, --script               Read script from stdin to use as key chords.\n"
        "    -S, --sort                 Sort key chords read from --key-chords, --script,\n"
        "                               or --transpile.\n"
        "    -m, --max-columns INT      Set the maximum menu columns to INT (defualt 5).\n"
        "    -p, --press KEY(s)         Press KEY(s) before dispalying menu.\n"
        "    -T, --transpile FILE       Transpile FILE to valid 'key_chords.h' syntax and\n"
        "                               print to stdout.\n"
        "    -k, --key-chords FILE      Use FILE for key chords rather than those\n"
        "                               precompiled.\n"
        "    -w, --menu-width INT       Set menu width to INT. Set to '-1' for a width\n"
        "                               equal to 1/2 of the screen width (default -1)\n"
        "    -g, --menu-gap INT         Set menu gap between top/bottom of screen to INT.\n"
        "                               Set to '-1' for a gap equal to 1/10th of the\n"
        "                               screen height (default -1).\n"
        "    --border-width INT         Set border width to INT (default 4).\n"
        "    --border-radius NUM        Set border radius to NUM degrees. 0 means no curve\n"
        "                               (default 0).\n"
        "    --wpadding INT             Set left and right padding around hint text to\n"
        "                               INT (default 6).\n"
        "    --hpadding INT             Set top and bottom padding around hint text to\n"
        "                               INT (default 2).\n"
        "    --table-padding INT        Set additional padding between the outermost cells\n"
        "                               and the border to INT. -1 = same as cell padding,\n"
        "                               0 = no additional padding (default -1).\n"
        "    --fg COLOR                 Set all menu foreground text to COLOR where color\n"
        "                               is some hex string i.e. '#F1CD39' (default unset).\n"
        "    --fg-key COLOR             Set foreground key to COLOR (default '#DCD7BA').\n"
        "    --fg-delimiter COLOR       Set foreground delimiter to COLOR (default '#525259').\n"
        "    --fg-prefix COLOR          Set foreground prefix to COLOR (default '#AF9FC9').\n"
        "    --fg-chord COLOR           Set foreground chord to COLOR (default '#DCD7BA').\n"
        "    --bg COLOR                 Set background to COLOR (default '#181616').\n"
        "    --bd COLOR                 Set border to COLOR (default '#7FB4CA').\n"
        "    --shell STRING             Set shell to STRING (default '/bin/sh').\n"
        "    --font STRING              Set font to STRING. Should be a valid Pango font\n"
        "                               description (default 'monospace, 14').\n"
        "    --implicit-keys STRING     Set implicit keys to STRING (default 'asdfghjkl;').\n"
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
menuParseArgs(Menu* menu, int* argc, char*** argv)
{
#define GET_ARG(arg)        ((*arg)[(optind == 1 ? optind : optind - 1)])

    assert(menu), assert(argc), assert(argv);

    int opt = '\0';
    static struct option longOpts[] = {
        /*                  no argument                 */
        { "help",           no_argument,        0, 'h' },
        { "version",        no_argument,        0, 'v' },
        { "debug",          no_argument,        0, 'd' },
        { "uwsm",           no_argument,        0, 'u' },
        { "top",            no_argument,        0, 't' },
        { "bottom",         no_argument,        0, 'b' },
        { "script",         no_argument,        0, 's' },
        { "sort",           no_argument,        0, 'S' },
        /*                  required argument           */
        { "delay",          required_argument,  0, 'D' },
        { "max-columns",    required_argument,  0, 'm' },
        { "press",          required_argument,  0, 'p' },
        { "transpile",      required_argument,  0, 'T' },
        { "key-chords",     required_argument,  0, 'k' },
        { "menu-width",     required_argument,  0, 'w' },
        { "menu-gap",       required_argument,  0, 'g' },
        { "border-width",   required_argument,  0, 0x090 },
        { "border-radius",  required_argument,  0, 0x091 },
        { "wpadding",       required_argument,  0, 0x092 },
        { "hpadding",       required_argument,  0, 0x093 },
        { "table-padding",  required_argument,  0, 0x094 },
        { "fg",             required_argument,  0, 0x095 },
        { "fg-key",         required_argument,  0, 0x096 },
        { "fg-delimiter",   required_argument,  0, 0x097 },
        { "fg-prefix",      required_argument,  0, 0x098 },
        { "fg-chord",       required_argument,  0, 0x099 },
        { "bg",             required_argument,  0, 0x100 },
        { "bd",             required_argument,  0, 0x101 },
        { "shell",          required_argument,  0, 0x102 },
        { "font",           required_argument,  0, 0x103 },
        { "implicit-keys",  required_argument,  0, 0x104 },
        { 0, 0, 0, 0 }
    };

    /* Don't let 'getopt' print errors. */
    opterr = 0;

    while (true)
    {

        opt = getopt_long(*argc, *argv, ":hvdtbsSD:m:p:T:c:w:g:", longOpts, NULL);
        if (opt < 0) break;

        switch (opt)
        {
        /* no argument */
        case 'h': usage(); exit(EXIT_FAILURE);
        case 'v': puts("wk v"VERSION); exit(EXIT_SUCCESS);
        case 'd': menu->debug = true; break;
        case 'u': menu->uwsmWrapper = true; break;
        case 't': menu->position = MENU_POS_TOP; break;
        case 'b': menu->position = MENU_POS_BOTTOM; break;
        case 's': menu->client.tryScript = true; break;
        case 'S': menu->sort = true; break;
        /* requires argument */
        case 'D':
        {
            int n = 0;
            if (!getInt(&n))
            {
                warnMsg("Could not convert '%s' into a integer.", optarg);
                warnMsg("Using default value of delay: %u.", menu->delay);
                break;
            }
            menu->delay = (uint32_t)n;
            break;
        }
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
                warnMsg("Using default value for menu-width: %u.", menu->menuWidth);
                break;
            }
            menu->menuWidth = n;
            break;
        }
        case 'g':
        {
            int n = 0;
            if (!getInt(&n))
            {
                warnMsg("Could not convert '%s' into a integer.", optarg);
                warnMsg("Using default value for menu-gap: %u.", menu->menuGap);
                break;
            }
            menu->menuGap = n;
            break;
        }
        /* border-width */
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
        /* border-radius */
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
        /* wpadding */
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
        /* hpadding */
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
        /* table-padding */
        case 0x094:
        {
            int n = 0;
            if (!getInt(&n))
            {
                warnMsg("Could not convert '%s' into a integer.", optarg);
                warnMsg("Using default value for table-padding: %d.", menu->tablePadding);
                break;
            }
            menu->tablePadding = n;
            break;
        }
        /* fg */
        case 0x095:
        {
            menuSetColor(menu, optarg, MENU_COLOR_KEY);
            menuSetColor(menu, optarg, MENU_COLOR_DELIMITER);
            menuSetColor(menu, optarg, MENU_COLOR_PREFIX);
            menuSetColor(menu, optarg, MENU_COLOR_CHORD);
            break;
        }
        /* fg-key */
        case 0x096: menuSetColor(menu, optarg, MENU_COLOR_KEY); break;
        /* fg-delimiter */
        case 0x097: menuSetColor(menu, optarg, MENU_COLOR_DELIMITER); break;
        /* fg-prefix */
        case 0x098: menuSetColor(menu, optarg, MENU_COLOR_PREFIX); break;
        /* fg-chord */
        case 0x099: menuSetColor(menu, optarg, MENU_COLOR_CHORD); break;
        /* bg */
        case 0x100: menuSetColor(menu, optarg, MENU_COLOR_BACKGROUND); break;
        /* bd */
        case 0x101: menuSetColor(menu, optarg, MENU_COLOR_BORDER); break;
        /* shell */
        case 0x102: menu->shell = optarg; break;
        /* font */
        case 0x103: menu->font = optarg; break;
        /* implicit keys */
        case 0x104: menu->implicitArrayKeys = optarg; break;
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
menuResetTimer(Menu* menu)
{
    assert(menu);

    if (menuIsDelayed(menu)) clock_gettime(CLOCK_MONOTONIC, &menu->timer);
}

void
menuSetColor(Menu* menu, const char* color, MenuColor colorType)
{
    assert(menu), assert(colorType < MENU_COLOR_LAST);

    if (!menuHexColorInitColor(&menu->colors[colorType], color)) warnMsg("Invalid color string: '%s'.", color);
}

static MenuStatus
spawnSync(const char* shell, const char* cmd)
{
    assert(shell), assert(cmd);

    setsid();
    char* exec[] = { strdup(shell), "-c", strdup(cmd), NULL, NULL };
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
menuSpawn(const Menu* menu, const String* cmd, bool sync)
{
    assert(menu), assert(cmd);

    if (stringIsEmpty(cmd)) return MENU_STATUS_EXIT_OK;

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
        const char* uwsm_prefix = "uwsm app -- ";
        size_t uwsm_prefix_len = strlen(uwsm_prefix);

        /* Calculate buffer size: command length + null terminator + optional prefix */
        size_t buffer_len = cmd->length + 1 + (menu->uwsmWrapper ? uwsm_prefix_len : 0);
        char buffer[buffer_len];

        if (menu->uwsmWrapper)
        {
            strcpy(buffer, uwsm_prefix);
            stringWriteToBuffer(cmd, buffer + uwsm_prefix_len);
        }
        else
        {
            stringWriteToBuffer(cmd, buffer);
        }

        printf("Running: `%s`\n", buffer);

        if (sync)
        {
            spawnSync(menu->shell, buffer);
        }
        else
        {
            spawnAsync(menu->shell, buffer);
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
menuStatusIsError(MenuStatus status)
{
    return status == MENU_STATUS_EXIT_SOFTWARE;
}

bool
menuStatusIsRunning(MenuStatus status)
{
    return status == MENU_STATUS_RUNNING || status == MENU_STATUS_DAMAGED;
}

bool
menuTryStdin(Menu* menu)
{
    assert(menu);

    Array* scriptArray = &menu->client.script;

    char* line = NULL;
    size_t lineCapacity = 0;
    ssize_t n;
    while ((n = getline(&line, &lineCapacity, stdin)) != -1)
    {
        if (n > 0) arrayAppendN(scriptArray, line, n);
    }

    free(line);
    arrayAppend(scriptArray, "");
    return n == -1 && feof(stdin);
}

