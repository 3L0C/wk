#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* local includes */
#include "arena.h"
#include "common.h"
#include "string.h"

void
errorMsg(const char* fmt, ...)
{
    assert(fmt);

    fprintf(stderr, "[ERROR] ");
    int     len = strlen(fmt);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc((fmt[len - 1] == ':' ? ' ' : '\n'), stderr);
}

const char*
getSeparator(int* count, const char* a, const char* b)
{
    assert(count);

    return ((*count)-- > 1 ? a : b);
}

bool
isUtf8ContByte(char byte)
{
    return (byte & 0xC0) == 0x80;
}

bool
isUtf8MultiByteStartByte(char byte)
{
    return (byte & 0x80) == 0x80 && (byte & 0xC0) != 0x80;
}

bool
isUtf8StartByte(char byte)
{
    return (byte & 0xC0) != 0x80;
}

String
readFileToArena(Arena* arena, const char* filepath)
{
    assert(arena), assert(filepath);

    String result = { 0 };
    FILE*  file   = fopen(filepath, "rb");
    if (!file)
    {
        errorMsg("Could not open file '%s'.", filepath);
        return result;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = ARENA_ALLOCATE(arena, char, fileSize + 1);

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize)
    {
        errorMsg("Could not read file '%s'.", filepath);
        fclose(file);
        return result;
    }

    buffer[bytesRead] = '\0';
    fclose(file);

    result.data   = buffer;
    result.length = bytesRead;
    return result;
}

void
warnMsg(const char* fmt, ...)
{
    assert(fmt);

    fprintf(stderr, "[WARNING] ");
    int     len = strlen(fmt);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc((fmt[len - 1] == ':' ? ' ' : '\n'), stderr);
}
