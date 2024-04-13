#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>


/* local includes */
#include "common.h"
#include "debug.h"
#include "menu.h"
#include "types.h"
#include "util.h"

void
calculateGrid(const uint32_t count, const uint32_t maxCols, uint32_t* rows, uint32_t* cols)
{
    assert(rows && cols);

    if (maxCols == 0 || maxCols >= count)
    {
        *rows = 1;
        *cols = count;
    }
    else
    {
        *rows = (count + maxCols - 1) / maxCols;
        *cols = (count + *rows - 1) / *rows;
    }
}

uint32_t
countKeyChords(const WkKeyChord* keyChords)
{
    assert(keyChords);

    uint32_t count = 0;
    while (keyChords[count].state == WK_KEY_CHORD_STATE_NOT_NULL) count++;

    return count;
}


static bool
modsEqual(const WkMods* a, const WkMods* b, bool checkShift)
{
    if (checkShift)
    {
        return (
            a->ctrl == b->ctrl &&
            a->alt == b->alt &&
            a->hyper == b->hyper &&
            a->shift == b->shift
        );
    }
    return (
        a->ctrl == b->ctrl &&
        a->alt == b->alt &&
        a->hyper == b->hyper
    );
}

static bool
isSpecialKey(const WkKeyChord* keyChord, WkKey* key)
{
    return (
        key->special != WK_SPECIAL_NONE &&
        keyChord->special == key->special &&
        modsEqual(&keyChord->mods, &key->mods, true)
    );
}

static bool
isKey(const WkKeyChord* keyChord, WkKey* key)
{
    if (isSpecialKey(keyChord, key)) return true;
    return (
        modsEqual(&keyChord->mods, &key->mods, false) &&
        keyChord->special == key->special &&
        strcmp(keyChord->key, key->key) == 0
    );
}

static WkStatus
handlePrefix(WkMenu* menu, const WkKeyChord* keyChord)
{
    debugMsg(menu->debug, "Found prefix.");

    menu->keyChords = keyChord->keyChords;
    countMenuKeyChords(menu);
    return WK_STATUS_DAMAGED;
}

static void
handleCommand(WkMenu* props, const WkKeyChord* keyChord)
{
    if (keyChord->flags.write)
    {
        printf("%s\n", keyChord->command);
        return;
    }
    spawn(props, keyChord->command, keyChord->flags.syncCommand);
}

static WkStatus
handleCommands(WkMenu* props, const WkKeyChord* keyChord)
{
    /* no command */
    if (!keyChord->command) return WK_STATUS_EXIT_OK;

    if (keyChord->before) spawn(props, keyChord->before, keyChord->flags.beforeSync);
    handleCommand(props, keyChord);
    if (keyChord->after) spawn(props, keyChord->after, keyChord->flags.afterSync);
    return keyChord->flags.keep ? WK_STATUS_RUNNING : WK_STATUS_EXIT_OK;
}

static WkStatus
pressKey(WkMenu* menu, const WkKeyChord* keyChord)
{
    assert(menu && keyChord);

    if (keyChord->keyChords) return handlePrefix(menu, keyChord);
    return handleCommands(menu, keyChord);
}

WkStatus
handleKeypress(WkMenu* menu, WkKey* key)
{
    uint32_t len = menu->keyChordCount;
    const WkKeyChord* keyChords = menu->keyChords;

    for (uint32_t i = 0; i < len; i++)
    {
        if (isKey(&keyChords[i], key))
        {
            if (menu->debug)
            {
                debugMsg(menu->debug, "Found match: '%s'.", keyChords[i].key);
                disassembleKeyChord(&keyChords[i], 0);
                debugKey(key);
            }
            return pressKey(menu, &keyChords[i]);
        }
    }

    if (menu->debug)
    {
        debugMsg(menu->debug, "Did not find a match for keypress.");
        debugKey(key);
    }

    return WK_STATUS_EXIT_SOFTWARE;
}

bool
isUtf8ContByte(char byte)
{
    return (byte & 0xC0) == 0x80;
}

bool
isUtf8StartByte(char byte)
{
    return (byte & 0xC0) != 0x80;
}

bool
isUtf8MultiByteStartByte(char byte)
{
    return (byte & 0x80) == 0x80 && (byte & 0xC0) != 0x80;
}

static WkStatus
spawnSync(const char* shell, const char* cmd)
{
    assert(shell && cmd);

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
    return WK_STATUS_EXIT_SOFTWARE;
}

static WkStatus
spawnAsync(const char* shell, const char* cmd)
{
    assert(shell && cmd);

    if (fork() == 0)
    {
        spawnSync(shell, cmd);
    }
    return WK_STATUS_EXIT_OK;
}

WkStatus
spawn(WkMenu* menu, const char* cmd, bool sync)
{
    assert(menu && cmd);

    pid_t child = fork();

    if (child == -1)
    {
        errorMsg("Could not fork process:");
        perror(NULL);
        return WK_STATUS_EXIT_SOFTWARE;
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
            return WK_STATUS_EXIT_SOFTWARE;
        }

        return WIFEXITED(status) ? WK_STATUS_EXIT_OK : WK_STATUS_EXIT_SOFTWARE;
    }

    wait(NULL);
    return WK_STATUS_EXIT_OK;
}

bool
statusIsError(WkStatus status)
{
    return status == WK_STATUS_EXIT_SOFTWARE;
}

bool
statusIsRunning(WkStatus status)
{
    return status == WK_STATUS_RUNNING || status == WK_STATUS_DAMAGED;
}
