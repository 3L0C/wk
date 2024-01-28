#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>

#include "common.h"
#include "debug.h"
#include "properties.h"
#include "types.h"
#include "util.h"
#include "window.h"

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

void
countChords(WkProperties* props)
{
    assert(props);

    const Chord* chords = props->chords;
    uint32_t* count = &props->chordCount;

    while (chords[*count].key) (*count)++;
}

static bool
isKey(const Chord* chord, Key* key)
{
    if (strcmp(chord->key, key->key) == 0) return true;
    return iscntrl(*key->key) && chord->mods == key->mods && chord->special == key->special;
}

static bool
testHook(const Chord* chord, bool nohook)
{
    return (!chord->unhook && !nohook && chord->command);
}

static void
setBeforeHook(Chord* chord, const char* hook)
{
    if (hook && testHook(chord, chord->nobefore))
    {
        chord->before = hook;
    }
}

static void
setAfterHook(Chord* chord, const char* hook)
{
    if (hook && testHook(chord, chord->noafter))
    {
        chord->after = hook;
    }
}

static void
setHooks(const Chord* chord)
{
    if (!chord->before || !chord->after) return;

    Chord* chords = chord->chords;
    for (int i = 0; chords[i].key; i++)
    {
        setBeforeHook(&chords[i], chord->before);
        setAfterHook(&chords[i], chord->after);
    }
}

static WkStatus
handlePrefix(WkProperties* props, const Chord* chord)
{
    debugMsg(props->debug, "Found prefix.");

    setHooks(chord);
    props->chords = chord->chords;
    return WK_STATUS_DAMAGED;
}

static void
handleCommand(WkProperties* props, const Chord* chord)
{
    if (chord->write)
    {
        printf("%s\n", chord->command);
        return;
    }
    spawn(props, chord->command, chord->async || chord->after != NULL);
}

static WkStatus
handleCommands(WkProperties* props, const Chord* chord)
{
    /* no command */
    if (!chord->command) return WK_STATUS_EXIT_OK;

    if (chord->before) spawn(props, chord->before, chord->async);
    handleCommand(props, chord);
    if (chord->after) spawn(props, chord->after, chord->async);
    return chord->keep ? WK_STATUS_DAMAGED : WK_STATUS_EXIT_OK;
}

static WkStatus
pressKey(WkProperties* props, const Chord* chord)
{
    assert(props && chord);

    if (chord->chords) return handlePrefix(props, chord);
    return handleCommands(props, chord);
}

WkStatus
handleKeypress(WkProperties* props, Key* key)
{
    uint32_t len = props->chordCount;
    const Chord* chords = props->chords;

    for (uint32_t i = 0; i < len; i++)
    {
        debugChord(&chords[i]);
        debugKey(key);
        if (isKey(&chords[i], key))
        {
            debugMsg(props->debug, "Found match: '%s'.", chords[i].key);
            return pressKey(props, &chords[i]);
        }
    }

    return WK_STATUS_EXIT_SOFTWARE;
}

bool
isUtf8StartByte(char byte)
{
    return (byte & 0xC0) != 0x80;
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
spawn(WkProperties* props, const char* cmd, bool async)
{
    assert(props && cmd);

    pid_t child = fork();

    if (child == -1)
    {
        errorMsg("Could not fork process:");
        perror(NULL);
        return WK_STATUS_EXIT_SOFTWARE;
    }

    if (child == 0)
    {
        if (props->xp && props->cleanupfp) props->cleanupfp(props->xp);
        if (async)
        {
            spawnAsync(props->shell, cmd);
        }
        else
        {
            spawnSync(props->shell, cmd);
        }
        exit(EX_OK);
    }

    if (!async)
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
