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

typedef uint8_t MenuOptArg;
enum
{
    OPT_ARG_BORDER_WIDTH  = 0x090,
    OPT_ARG_BORDER_RADIUS = 0x091,
    OPT_ARG_WPADDING      = 0x092,
    OPT_ARG_HPADDING      = 0x093,
    OPT_ARG_TABLE_PADDING = 0x094,
    OPT_ARG_FG            = 0x095,
    OPT_ARG_FG_KEY        = 0x096,
    OPT_ARG_FG_DELIMITER  = 0x097,
    OPT_ARG_FG_PREFIX     = 0x098,
    OPT_ARG_FG_CHORD      = 0x099,
    OPT_ARG_FG_TITLE      = 0x100,
    OPT_ARG_BG            = 0x101,
    OPT_ARG_BD            = 0x102,
    OPT_ARG_SHELL         = 0x103,
    OPT_ARG_FONT          = 0x104,
    OPT_ARG_IMPLICIT_KEYS = 0x105,
    OPT_ARG_WRAP_CMD      = 0x106,
    OPT_ARG_TITLE         = 0x107,
    OPT_ARG_TITLE_FONT    = 0x108,
};

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
    arrayFree(&menu->userVars);
    arenaFree(&menu->arena);
}

static MenuStatus
menuHandlePrefix(Menu* menu, KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    debugMsg(menu->debug, "Found prefix.");

    menu->keyChords = &keyChord->keyChords;
    if (!stringIsEmpty(&keyChord->title))
    {
        menu->title = stringToCString(&menu->arena, &keyChord->title);
    }

    return MENU_STATUS_DAMAGED;
}

static const String*
getWrapper(const Menu* menu, const KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    if (chordFlagIsActive(keyChord->flags, FLAG_UNWRAP)) return NULL;
    if (!stringIsEmpty(&keyChord->wrapCmd)) return &keyChord->wrapCmd;
    if (!stringIsEmpty(&menu->wrapCmd)) return &menu->wrapCmd;
    return NULL;
}

static String
getCmd(const Menu* menu, const KeyChord* keyChord)
{
    const String* wrapper = getWrapper(menu, keyChord);

    String cmd = stringInit();

    if (wrapper != NULL)
    {
        stringAppendString(&cmd, wrapper);
        stringAppendCString(&cmd, " ");
    }

    stringAppendString(&cmd, &keyChord->command);

    return cmd;
}

static void
menuHandleCommand(Menu* menu, KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    String cmd = getCmd(menu, keyChord);

    /* Nothing to print */
    if (cmd.length == 0) return;

    if (chordFlagIsActive(keyChord->flags, FLAG_WRITE))
    {
        char buffer[cmd.length + 1];
        stringWriteToBuffer(&cmd, buffer);

        printf("%s\n", buffer);
        return;
    }

    menuSpawn(menu, keyChord, &cmd, chordFlagIsActive(keyChord->flags, FLAG_SYNC_COMMAND));
}

static MenuStatus
menuHandleCommands(Menu* menu, KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    menuSpawn(
        menu,
        keyChord,
        &keyChord->before,
        chordFlagIsActive(keyChord->flags, FLAG_SYNC_BEFORE));
    menuHandleCommand(menu, keyChord);
    menuSpawn(
        menu,
        keyChord,
        &keyChord->after,
        chordFlagIsActive(keyChord->flags, FLAG_SYNC_AFTER));

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
    int          count = sscanf(color, "#%2x%2x%2x%2x", &r, &g, &b, &a);

    if (count != 3 && count != 4) return false;

    hexColor->hex = color;
    hexColor->r   = r;
    hexColor->g   = g;
    hexColor->b   = b;
    hexColor->a   = a;

    return true;
}

static void
menuHexColorInitColors(MenuHexColor* hexColors)
{
    assert(hexColors);

    static const char* defaultColors[MENU_COLOR_LAST] = {
        [MENU_COLOR_KEY]        = "#DCD7BA",
        [MENU_COLOR_DELIMITER]  = "#525259",
        [MENU_COLOR_PREFIX]     = "#AF9FC9",
        [MENU_COLOR_CHORD]      = "#DCD7BA",
        [MENU_COLOR_TITLE]      = "#DCD7BA",
        [MENU_COLOR_BACKGROUND] = "#181616",
        [MENU_COLOR_BORDER]     = "#7FB4CA",
    };

    const char* colors[MENU_COLOR_LAST] = {
        [MENU_COLOR_KEY]        = foreground[FOREGROUND_COLOR_KEY],
        [MENU_COLOR_DELIMITER]  = foreground[FOREGROUND_COLOR_DELIMITER],
        [MENU_COLOR_PREFIX]     = foreground[FOREGROUND_COLOR_PREFIX],
        [MENU_COLOR_CHORD]      = foreground[FOREGROUND_COLOR_CHORD],
        [MENU_COLOR_TITLE]      = foreground[FOREGROUND_COLOR_TITLE],
        [MENU_COLOR_BACKGROUND] = background,
        [MENU_COLOR_BORDER]     = border,
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
            case MENU_COLOR_TITLE: colorType = "title"; break;
            default: colorType = "UNKNOWN"; break;
            }
            fprintf(stderr, "setting %s to '%s'.\n", colorType, defaultColors[i]);
            menuHexColorInitColor(&hexColors[i], defaultColors[i]);
        }
    }
}

void
menuInit(Menu* menu)
{
    assert(menu);

    menu->delimiter         = delimiter;
    menu->shell             = shell;
    menu->title             = NULL;
    menu->font              = font;
    menu->titleFont         = titleFont;
    menu->implicitArrayKeys = implicitArrayKeys;
    menu->borderRadius      = borderRadius;
    menuHexColorInitColors(menu->colors);
    menu->client.keys      = NULL;
    menu->client.transpile = NULL;
    menu->client.wksFile   = NULL;
    menu->client.tryScript = false;
    menu->client.script    = ARRAY_INIT(char);
    clock_gettime(CLOCK_MONOTONIC, &menu->timer);
    menu->userVars          = ARRAY_INIT(UserVar);
    menu->compiledKeyChords = ARRAY_INIT(KeyChord);
    menu->builtinKeyChords  = &builtinKeyChords;
    menu->keyChords         = &builtinKeyChords;
    menu->keyChordsHead     = &builtinKeyChords;
    menu->cleanupfp         = NULL;
    menu->xp                = NULL;
    arenaInit(&menu->arena);

    menu->maxCols      = maxCols;
    menu->menuWidth    = menuWidth;
    menu->menuGap      = menuGap;
    menu->wpadding     = widthPadding;
    menu->hpadding     = heightPadding;
    menu->tablePadding = tablePadding;
    menu->cellHeight   = 0;
    menu->titleHeight  = 0;
    menu->rows         = 0;
    menu->cols         = 0;
    menu->width        = 0;
    menu->height       = 0;
    menu->borderWidth  = borderWidth;
    menu->delay        = delay;

    menu->position = (menuPosition ? MENU_POS_TOP : MENU_POS_BOTTOM);
    menu->debug    = false;
    menu->sort     = false;
    menu->dirty    = true;
    menu->wrapCmd  = stringInitFromChar(wrapCmd);
}

bool
menuIsDelayed(Menu* menu)
{
    assert(menu);
    if (!menu->delay) return false;

    static struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    long elapsedTime =
        (((now.tv_sec - menu->timer.tv_sec) * 1000) +
         ((now.tv_nsec - menu->timer.tv_nsec) / 1000000));

    return elapsedTime < menu->delay;
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
        "    --fg-title COLOR           Set foreground title to COLOR (default '#DCD7BA').\n"
        "    --bg COLOR                 Set background to COLOR (default '#181616').\n"
        "    --bd COLOR                 Set border to COLOR (default '#7FB4CA').\n"
        "    --shell STRING             Set shell to STRING (default '/bin/sh').\n"
        "    --title STRING             Set the menu title to STRING (default '')\n"
        "    --title-font STRING        Set title font to STRING. Should be a valid Pango font\n"
        "                               description (default 'sans-serif, 16')\n"
        "    --font STRING              Set font to STRING. Should be a valid Pango font\n"
        "                               description (default 'monospace, 14').\n"
        "    --implicit-keys STRING     Set implicit keys to STRING (default 'asdfghjkl;').\n"
        "    --wrap-cmd STRING          Wrap all commands with STRING, i.e., \n"
        "                                   /bin/sh -c STRING cmd\n"
        "                               This does not apply to hooks (default \"\").\n"
        "\n"
        "run `man 1 wk` for more info on each option.\n",
        stderr);
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
#define GET_ARG(arg) ((*arg)[(optind == 1 ? optind : optind - 1)])

    assert(menu), assert(argc), assert(argv);

    int opt = '\0';

    static struct option longOpts[] = {
        /*                  no argument                 */
        { "help",          no_argument,       0, 'h'                   },
        { "version",       no_argument,       0, 'v'                   },
        { "debug",         no_argument,       0, 'd'                   },
        { "top",           no_argument,       0, 't'                   },
        { "bottom",        no_argument,       0, 'b'                   },
        { "script",        no_argument,       0, 's'                   },
        { "sort",          no_argument,       0, 'S'                   },
        /*                  required argument           */
        { "delay",         required_argument, 0, 'D'                   },
        { "max-columns",   required_argument, 0, 'm'                   },
        { "press",         required_argument, 0, 'p'                   },
        { "transpile",     required_argument, 0, 'T'                   },
        { "key-chords",    required_argument, 0, 'k'                   },
        { "menu-width",    required_argument, 0, 'w'                   },
        { "menu-gap",      required_argument, 0, 'g'                   },
        { "border-width",  required_argument, 0, OPT_ARG_BORDER_WIDTH  },
        { "border-radius", required_argument, 0, OPT_ARG_BORDER_RADIUS },
        { "wpadding",      required_argument, 0, OPT_ARG_WPADDING      },
        { "hpadding",      required_argument, 0, OPT_ARG_HPADDING      },
        { "table-padding", required_argument, 0, OPT_ARG_TABLE_PADDING },
        { "fg",            required_argument, 0, OPT_ARG_FG            },
        { "fg-key",        required_argument, 0, OPT_ARG_FG_KEY        },
        { "fg-delimiter",  required_argument, 0, OPT_ARG_FG_DELIMITER  },
        { "fg-prefix",     required_argument, 0, OPT_ARG_FG_PREFIX     },
        { "fg-chord",      required_argument, 0, OPT_ARG_FG_CHORD      },
        { "fg-title",      required_argument, 0, OPT_ARG_FG_TITLE      },
        { "bg",            required_argument, 0, OPT_ARG_BG            },
        { "bd",            required_argument, 0, OPT_ARG_BD            },
        { "shell",         required_argument, 0, OPT_ARG_SHELL         },
        { "font",          required_argument, 0, OPT_ARG_FONT          },
        { "implicit-keys", required_argument, 0, OPT_ARG_IMPLICIT_KEYS },
        { "wrap-cmd",      required_argument, 0, OPT_ARG_WRAP_CMD      },
        { "title",         required_argument, 0, OPT_ARG_TITLE         },
        { "title-font",    required_argument, 0, OPT_ARG_TITLE_FONT    },
        { 0,               0,                 0, 0                     }
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
        case 'v': puts("wk v" VERSION); exit(EXIT_SUCCESS);
        case 'd': menu->debug = true; break;
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
        case OPT_ARG_BORDER_WIDTH:
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
        case OPT_ARG_BORDER_RADIUS:
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
        case OPT_ARG_WPADDING:
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
        case OPT_ARG_HPADDING:
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
        case OPT_ARG_TABLE_PADDING:
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
        case OPT_ARG_FG:
        {
            menuSetColor(menu, optarg, MENU_COLOR_KEY);
            menuSetColor(menu, optarg, MENU_COLOR_DELIMITER);
            menuSetColor(menu, optarg, MENU_COLOR_PREFIX);
            menuSetColor(menu, optarg, MENU_COLOR_CHORD);
            break;
        }
        case OPT_ARG_FG_KEY: menuSetColor(menu, optarg, MENU_COLOR_KEY); break;
        case OPT_ARG_FG_DELIMITER: menuSetColor(menu, optarg, MENU_COLOR_DELIMITER); break;
        case OPT_ARG_FG_PREFIX: menuSetColor(menu, optarg, MENU_COLOR_PREFIX); break;
        case OPT_ARG_FG_CHORD: menuSetColor(menu, optarg, MENU_COLOR_CHORD); break;
        case OPT_ARG_FG_TITLE: menuSetColor(menu, optarg, MENU_COLOR_TITLE); break;
        case OPT_ARG_BG: menuSetColor(menu, optarg, MENU_COLOR_BACKGROUND); break;
        case OPT_ARG_BD: menuSetColor(menu, optarg, MENU_COLOR_BORDER); break;
        case OPT_ARG_SHELL: menu->shell = optarg; break;
        case OPT_ARG_FONT: menu->font = optarg; break;
        case OPT_ARG_IMPLICIT_KEYS: menu->implicitArrayKeys = optarg; break;
        case OPT_ARG_WRAP_CMD: menuSetWrapCmd(menu, optarg); break;
        case OPT_ARG_TITLE: menu->title = optarg; break;
        case OPT_ARG_TITLE_FONT: menu->titleFont = optarg; break;
        /* Errors */
        case '?':
        {
            usage();
            errorMsg("Unrecognized option: '%s'.", GET_ARG(argv));
            exit(EXIT_FAILURE);
        }
        case ':':
        {
            usage();
            errorMsg("'%s' requires an argument but none given.", GET_ARG(argv));
            exit(EXIT_FAILURE);
        }
        default:
        {
            usage();
            exit(EXIT_FAILURE);
            break;
        }
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

    if (!menuHexColorInitColor(&menu->colors[colorType], color))
    {
        warnMsg("Invalid color string: '%s'.", color);
    }
}

void
menuSetWrapCmd(Menu* menu, const char* cmd)
{
    assert(menu);

    if (!stringIsEmpty(&menu->wrapCmd)) stringFree(&menu->wrapCmd);
    menu->wrapCmd = stringInitFromChar(cmd);
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
menuSpawn(const Menu* menu, const KeyChord* keyChord, const String* cmd, bool sync)
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

        if (cmd->length == 0) exit(EX_OK);

        char buffer[cmd->length + 1];
        stringWriteToBuffer(cmd, buffer);

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

    char*   line         = NULL;
    size_t  lineCapacity = 0;
    ssize_t n;
    while ((n = getline(&line, &lineCapacity, stdin)) != -1)
    {
        if (n > 0) arrayAppendN(scriptArray, line, n);
    }

    free(line);
    arrayAppend(scriptArray, "");
    return n == -1 && feof(stdin);
}
