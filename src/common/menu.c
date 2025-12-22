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
#include <time.h>
#include <unistd.h>

#ifdef WK_X11_BACKEND
#include "runtime/x11/window.h"
#endif

#ifdef WK_WAYLAND_BACKEND
#include "runtime/wayland/wayland.h"
#endif

/* config includes */
#include "config/config.h"

/* common includes */
#include "arena.h"
#include "common.h"
#include "debug.h"
#include "key_chord.h"
#include "span.h"
#include "stack.h"
#include "string.h"
#include "vector.h"

/* local includes */
#include "menu.h"

/* compiler includes */
#include "compiler/common.h"

typedef uint8_t MenuOptArg;
enum
{
    OPT_ARG_BORDER_WIDTH = 0x090,
    OPT_ARG_BORDER_RADIUS,
    OPT_ARG_WPADDING,
    OPT_ARG_HPADDING,
    OPT_ARG_TABLE_PADDING,
    OPT_ARG_FG,
    OPT_ARG_FG_KEY,
    OPT_ARG_FG_DELIMITER,
    OPT_ARG_FG_PREFIX,
    OPT_ARG_FG_CHORD,
    OPT_ARG_FG_TITLE,
    OPT_ARG_FG_GOTO,
    OPT_ARG_BG,
    OPT_ARG_BD,
    OPT_ARG_SHELL,
    OPT_ARG_FONT,
    OPT_ARG_IMPLICIT_KEYS,
    OPT_ARG_WRAP_CMD,
    OPT_ARG_TITLE,
    OPT_ARG_TITLE_FONT,
    OPT_ARG_KEEP_DELAY,
};

int
menuDisplay(Menu* menu)
{
    assert(menu);

    if (SPAN_LENGTH(menu->keyChords) == 0)
    {
        warnMsg("No key chords to display...");
        return EX_DATAERR;
    }

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

    keyChordsFree(&menu->compiledKeyChords);
    vectorFree(&menu->userVars);
    arenaFree(&menu->arena);
}

static MenuStatus
menuHandlePrefix(Menu* menu, KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    debugMsg(menu->debug, "Found prefix.");

    menu->keyChords = &keyChord->keyChords;
    if (propIsSet(keyChord, KC_PROP_TITLE))
    {
        const String* title = propStringConst(keyChord, KC_PROP_TITLE);
        menu->title         = title->data;
    }

    return MENU_STATUS_DAMAGED;
}

static MenuStatus
menuHandleGoto(Menu* menu, KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    static Stack visitedChords = {
        .data        = NULL,
        .length      = 0,
        .capacity    = 0,
        .elementSize = sizeof(const KeyChord*)
    };

    vectorForEach(&visitedChords, const KeyChord*, visited)
    {
        if (*visited == keyChord)
        {
            errorMsg("Infinite @goto recursion detected.");
            return MENU_STATUS_EXIT_SOFTWARE;
        }
    }

    stackPush(&visitedChords, &keyChord);

    /* Reset to root */
    menu->keyChords = menu->keyChordsHead;
    menu->title     = menu->rootTitle;

    const String* gotoPath = propStringConst(keyChord, KC_PROP_GOTO);
    MenuStatus    status;

    if (stringIsEmpty(gotoPath))
    {
        debugMsg(menu->debug, "@goto: navigating to root.");
        status = MENU_STATUS_DAMAGED;
    }
    else
    {
        debugMsg(menu->debug, "@goto: navigating to '%s'.", gotoPath->data);
        status = menuHandlePath(menu, gotoPath->data);
    }

    stackPop(&visitedChords);

    return status;
}

static void
menuHandleCommand(Menu* menu, KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    if (chordFlagIsActive(keyChord->flags, FLAG_WRITE))
    {
        const String* cmd = propStringConst(keyChord, KC_PROP_COMMAND);
        if (cmd) printf("%s\n", cmd->data);
        return;
    }

    const char* wrapData = NULL;
    size_t      wrapLen  = 0;

    if (!chordFlagIsActive(keyChord->flags, FLAG_UNWRAP))
    {
        if (propIsSet(keyChord, KC_PROP_WRAP_CMD))
        {
            const String* wrapStr = propStringConst(keyChord, KC_PROP_WRAP_CMD);
            if (!stringIsEmpty(wrapStr))
            {
                wrapData = wrapStr->data;
                wrapLen  = wrapStr->length;
            }
        }
        else if (menu->wrapCmd && menu->wrapCmd[0])
        {
            wrapData = menu->wrapCmd;
            wrapLen  = strlen(menu->wrapCmd);
        }
    }

    const char*   cmdData = NULL;
    size_t        cmdLen  = 0;
    const String* cmdStr  = propStringConst(keyChord, KC_PROP_COMMAND);
    if (!stringIsEmpty(cmdStr))
    {
        cmdData = cmdStr->data;
        cmdLen  = cmdStr->length;
    }

    if (wrapLen == 0 && cmdLen == 0) return;

    bool   needsSpace = (wrapLen > 0 && cmdLen > 0);
    size_t totalLen   = wrapLen + cmdLen + (needsSpace ? 1 : 0);
    char   buffer[totalLen + 1];
    char*  p = buffer;

    if (wrapLen > 0)
    {
        memcpy(p, wrapData, wrapLen);
        p += wrapLen;
        if (needsSpace) *p++ = ' ';
    }
    if (cmdLen > 0)
    {
        memcpy(p, cmdData, cmdLen);
        p += cmdLen;
    }
    *p = '\0';

    String cmd = { .data = buffer, .length = totalLen };
    menuSpawn(menu, keyChord, &cmd, chordFlagIsActive(keyChord->flags, FLAG_SYNC_COMMAND));
}

static MenuStatus
menuHandleCommands(Menu* menu, KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    menuSpawn(
        menu,
        keyChord,
        propStringConst(keyChord, KC_PROP_BEFORE),
        chordFlagIsActive(keyChord->flags, FLAG_SYNC_BEFORE));
    menuHandleCommand(menu, keyChord);
    menuSpawn(
        menu,
        keyChord,
        propStringConst(keyChord, KC_PROP_AFTER),
        chordFlagIsActive(keyChord->flags, FLAG_SYNC_AFTER));

    /* If chord has +keep flag and a command to execute, sleep to allow
     * compositor to process the ungrab before the command executes */
    if (chordFlagIsActive(keyChord->flags, FLAG_KEEP) && propIsSet(keyChord, KC_PROP_COMMAND))
    {
        struct timespec sleep_duration = {
            .tv_sec  = 0,
            .tv_nsec = menu->keepDelay * 1000000
        };
        nanosleep(&sleep_duration, NULL);
    }

    return chordFlagIsActive(keyChord->flags, FLAG_KEEP) ? MENU_STATUS_RUNNING : MENU_STATUS_EXIT_OK;
}

static MenuStatus
menuPressKey(Menu* menu, KeyChord* keyChord)
{
    assert(menu), assert(keyChord);

    if (propIsSet(keyChord, KC_PROP_GOTO))
    {
        return menuHandleGoto(menu, keyChord);
    }
    if (keyChord->keyChords.count != 0)
    {
        return menuHandlePrefix(menu, keyChord);
    }
    return menuHandleCommands(menu, keyChord);
}

MenuStatus
menuHandleKeypress(Menu* menu, const Key* key)
{
    assert(menu), assert(key);

    spanForEach(menu->keyChords, KeyChord, keyChord)
    {
        if (keyIsEqual(&keyChord->key, key))
        {
            if (menu->debug)
            {
                debugMsg(menu->debug, "Found match: '%s'.", keyChord->key.repr.data);
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

MenuStatus
menuHandlePath(Menu* menu, const char* path)
{
    assert(menu), assert(path);

    Vector keys = VECTOR_INIT(Key);

    if (!compileKeys(&menu->arena, path, &keys))
    {
        vectorForEach(&keys, Key, key) { keyFree(key); }
        vectorFree(&keys);
        return MENU_STATUS_EXIT_SOFTWARE;
    }

    MenuStatus status = MENU_STATUS_RUNNING;

    vectorForEach(&keys, Key, key)
    {
        if (menu->debug)
        {
            debugMsg(menu->debug, "Pressing key: '%s'.", key->repr.data);
        }

        status = menuHandleKeypress(menu, key);

        if (!menuStatusIsRunning(status)) break;
    }

    vectorForEach(&keys, Key, key) { keyFree(key); }
    vectorFree(&keys);

    return status;
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
        [MENU_COLOR_GOTO]       = "#E6C384",
        [MENU_COLOR_BACKGROUND] = "#181616",
        [MENU_COLOR_BORDER]     = "#7FB4CA",
    };

    const char* colors[MENU_COLOR_LAST] = {
        [MENU_COLOR_KEY]        = foreground[FOREGROUND_COLOR_KEY],
        [MENU_COLOR_DELIMITER]  = foreground[FOREGROUND_COLOR_DELIMITER],
        [MENU_COLOR_PREFIX]     = foreground[FOREGROUND_COLOR_PREFIX],
        [MENU_COLOR_CHORD]      = foreground[FOREGROUND_COLOR_CHORD],
        [MENU_COLOR_TITLE]      = foreground[FOREGROUND_COLOR_TITLE],
        [MENU_COLOR_GOTO]       = foreground[FOREGROUND_COLOR_GOTO],
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
            case MENU_COLOR_GOTO: colorType = "goto"; break;
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
    menu->rootTitle         = NULL;
    menu->font              = font;
    menu->titleFont         = titleFont;
    menu->implicitArrayKeys = implicitArrayKeys;
    menu->borderRadius      = borderRadius;
    menuHexColorInitColors(menu->colors);
    menu->client.keys      = NULL;
    menu->client.transpile = NULL;
    menu->client.wksFile   = NULL;
    menu->client.tryScript = false;
    menu->client.script    = VECTOR_INIT(char);
    clock_gettime(CLOCK_MONOTONIC, &menu->timer);
    menu->userVars          = VECTOR_INIT(UserVar);
    menu->compiledKeyChords = SPAN_EMPTY;
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
    menu->keepDelay    = keepDelay;

    menu->position = (menuPosition ? MENU_POS_TOP : MENU_POS_BOTTOM);
    menu->debug    = false;
    menu->sort     = true;
    menu->dirty    = true;
    menu->wrapCmd  = wrapCmd;
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
        "    --keep-delay INT           Delay in milliseconds after ungrab before command\n"
        "                               execution for +keep chords (default 75 ms).\n"
        "    -t, --top                  Position menu at top of screen.\n"
        "    -b, --bottom               Position menu at bottom of screen.\n"
        "    -s, --script               Read script from stdin to use as key chords.\n"
        "    -U, --unsorted             Disable sorting of key chords (sorted by default).\n"
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
        "    --fg-goto COLOR            Set foreground goto to COLOR (default '#E6C384').\n"
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
        { "unsorted",      no_argument,       0, 'U'                   },
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
        { "fg-goto",       required_argument, 0, OPT_ARG_FG_GOTO       },
        { "bg",            required_argument, 0, OPT_ARG_BG            },
        { "bd",            required_argument, 0, OPT_ARG_BD            },
        { "shell",         required_argument, 0, OPT_ARG_SHELL         },
        { "font",          required_argument, 0, OPT_ARG_FONT          },
        { "implicit-keys", required_argument, 0, OPT_ARG_IMPLICIT_KEYS },
        { "wrap-cmd",      required_argument, 0, OPT_ARG_WRAP_CMD      },
        { "title",         required_argument, 0, OPT_ARG_TITLE         },
        { "title-font",    required_argument, 0, OPT_ARG_TITLE_FONT    },
        { "keep-delay",    required_argument, 0, OPT_ARG_KEEP_DELAY    },
        { 0,               0,                 0, 0                     }
    };

    /* Don't let 'getopt' print errors. */
    opterr = 0;

    while (true)
    {

        opt = getopt_long(*argc, *argv, ":hvdtbsUD:m:p:T:c:w:g:", longOpts, NULL);
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
        case 'U': menu->sort = false; break;
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
            menuSetColor(menu, optarg, MENU_COLOR_TITLE);
            menuSetColor(menu, optarg, MENU_COLOR_GOTO);
            break;
        }
        case OPT_ARG_FG_KEY: menuSetColor(menu, optarg, MENU_COLOR_KEY); break;
        case OPT_ARG_FG_DELIMITER: menuSetColor(menu, optarg, MENU_COLOR_DELIMITER); break;
        case OPT_ARG_FG_PREFIX: menuSetColor(menu, optarg, MENU_COLOR_PREFIX); break;
        case OPT_ARG_FG_CHORD: menuSetColor(menu, optarg, MENU_COLOR_CHORD); break;
        case OPT_ARG_FG_TITLE: menuSetColor(menu, optarg, MENU_COLOR_TITLE); break;
        case OPT_ARG_FG_GOTO: menuSetColor(menu, optarg, MENU_COLOR_GOTO); break;
        case OPT_ARG_BG: menuSetColor(menu, optarg, MENU_COLOR_BACKGROUND); break;
        case OPT_ARG_BD: menuSetColor(menu, optarg, MENU_COLOR_BORDER); break;
        case OPT_ARG_SHELL: menu->shell = optarg; break;
        case OPT_ARG_FONT: menu->font = optarg; break;
        case OPT_ARG_IMPLICIT_KEYS: menu->implicitArrayKeys = optarg; break;
        case OPT_ARG_WRAP_CMD: menuSetWrapCmd(menu, optarg); break;
        case OPT_ARG_TITLE: menu->title = menu->rootTitle = optarg; break;
        case OPT_ARG_TITLE_FONT: menu->titleFont = optarg; break;
        case OPT_ARG_KEEP_DELAY:
        {
            int n = 0;
            if (!getInt(&n))
            {
                warnMsg("Could not convert '%s' into an integer.", optarg);
                warnMsg("Using default value for keep-delay: %u.", menu->keepDelay);
                break;
            }
            menu->keepDelay = (uint32_t)n;
            break;
        }
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
    assert(menu), assert(color), assert(colorType < MENU_COLOR_LAST);

    if (!menuHexColorInitColor(&menu->colors[colorType], color))
    {
        warnMsg("Invalid color string: '%s'.", color);
    }
}

void
menuSetWrapCmd(Menu* menu, const char* cmd)
{
    assert(menu);
    menu->wrapCmd = cmd;
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
    assert(menu);

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

        if (sync)
        {
            spawnSync(menu->shell, cmd->data);
        }
        else
        {
            spawnAsync(menu->shell, cmd->data);
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

    Vector* scriptVector = &menu->client.script;

    char*   line         = NULL;
    size_t  lineCapacity = 0;
    ssize_t n;
    while ((n = getline(&line, &lineCapacity, stdin)) != -1)
    {
        if (n > 0) vectorAppendN(scriptVector, line, n);
    }

    free(line);
    vectorAppend(scriptVector, "");
    return n == -1 && feof(stdin);
}
