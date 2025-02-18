#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* local */
#include "common.h"
#include "arena.h"

void
errorMsg(const char* fmt, ...)
{
    assert(fmt);

    fprintf(stderr, "[ERROR] ");
    int len = strlen(fmt); /* 1 = '\0' */
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc((fmt[len - 1] == ':' ? ' ' : '\n'), stderr);
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

char*
readFile(Arena* arena, const char* filepath)
{
    assert(arena), assert(filepath);

    FILE* file = fopen(filepath, "rb");
    if (!file)
    {
        errorMsg("Could not open file '%s'.", filepath);
        return NULL;
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
        return NULL;
    }

    buffer[bytesRead] = '\0';
    fclose(file);

    return buffer;
}

void
warnMsg(const char* fmt, ...)
{
    assert(fmt);

    fprintf(stderr, "[WARNING] ");
    int len = strlen(fmt); /* 1 = '\0' */
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc((fmt[len - 1] == ':' ? ' ' : '\n'), stderr);
}
