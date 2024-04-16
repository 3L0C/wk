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
#include "key_chord.h"
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
countKeyChords(const KeyChord* keyChords)
{
    assert(keyChords);

    uint32_t count = 0;
    while (keyChords[count].state == KEY_CHORD_STATE_NOT_NULL) count++;

    return count;
}


static bool
modsEqual(const KeyChordMods* a, const KeyChordMods* b, bool checkShift)
{
    assert(a && b);

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
isSpecialKey(const KeyChord* keyChord, const KeyChordKey* key)
{
    assert(keyChord && key);

    return (
        key->special != SPECIAL_KEY_NONE &&
        keyChord->key.special == key->special &&
        modsEqual(&keyChord->key.mods, &key->mods, true)
    );
}

static bool
isKey(const KeyChord* keyChord, const KeyChordKey* key)
{
    assert(keyChord && key);

    if (isSpecialKey(keyChord, key)) return true;
    return (
        modsEqual(&keyChord->key.mods, &key->mods, false) &&
        keyChord->key.special == key->special &&
        strcmp(keyChord->key.repr, key->repr) == 0
    );
}

static MenuStatus
handlePrefix(Menu* menu, const KeyChord* keyChord)
{
    assert(menu && keyChord);

    debugMsg(menu->debug, "Found prefix.");

    menu->keyChords = keyChord->keyChords;
    countMenuKeyChords(menu);
    return MENU_STATUS_DAMAGED;
}

static void
handleCommand(Menu* menu, const KeyChord* keyChord)
{
    assert(menu && keyChord);

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
    assert(menu && keyChord);

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
    assert(menu && keyChord);

    if (keyChord->keyChords) return handlePrefix(menu, keyChord);
    return handleCommands(menu, keyChord);
}

MenuStatus
handleKeypress(Menu* menu, const KeyChordKey* key)
{
    assert(menu && key);

    uint32_t len = menu->keyChordCount;
    const KeyChord* keyChords = menu->keyChords;

    for (uint32_t i = 0; i < len; i++)
    {
        if (isKey(&keyChords[i], key))
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

static MenuStatus
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
    return MENU_STATUS_EXIT_SOFTWARE;
}

static MenuStatus
spawnAsync(const char* shell, const char* cmd)
{
    assert(shell && cmd);

    if (fork() == 0)
    {
        spawnSync(shell, cmd);
    }
    return MENU_STATUS_EXIT_OK;
}

MenuStatus
spawn(const Menu* menu, const char* cmd, bool sync)
{
    assert(menu && cmd);

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
