#include <assert.h>
#include <ctype.h>
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
    setHooks(chord);
    props->chords = chord->chords;
    debugMsg(props->debug, "Found prefix.");
    return WK_STATUS_DAMAGED;
}

static WkStatus
handleCommand(WkProperties* props, const Chord* chord)
{
    /* before */
    if (chord->before)
    {
        spawnAsync(props->shell, chord->before, props->cleanupfp, props->xp);
    }
    /* command with after hook */
    if (chord->command && chord->after)
    {
        debugMsg(props->debug, "Found command.");
        spawnAsync(props->shell, chord->command, props->cleanupfp, props->xp);
        spawnAsync(props->shell, chord->after, props->cleanupfp, props->xp);
        return WK_STATUS_EXIT_OK;
    }
    /* command no hook */
    if (chord->command)
    {
        debugMsg(props->debug, "Found command.");
        spawn(props->shell, chord->command);
        return WK_STATUS_EXIT_OK;
    }
    debugMsg(props->debug, "No match.");
    return WK_STATUS_EXIT_SOFTWARE;
}

static WkStatus
pressKey(WkProperties* props, const Chord* chord)
{
    assert(props && chord);

    if (chord->chords) return handlePrefix(props, chord);
    return handleCommand(props, chord);
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

WkStatus
spawn(const char* shell, const char* cmd)
{
    if (fork() == 0)
    {
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
    return WK_STATUS_EXIT_OK;
}

WkStatus
spawnAsync(const char* shell, const char* cmd, CleanupFP fp, void* xp)
{
    WkStatus result = WK_STATUS_EXIT_OK;
    if (fork() == 0) {
        if (xp && fp) fp(xp);
        result = spawn(shell, cmd);
        exit(EX_OK);
    }
    wait(NULL);
    return result;
}

