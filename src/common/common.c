#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* local */
#include "common.h"
#include "memory.h"

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
readFile(const char* filepath)
{
    assert(filepath);

    FILE* file = fopen(filepath, "rb");
    if (!file)
    {
        errorMsg("Could not open file '%s'.", filepath);
        goto fail;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = ALLOCATE(char, fileSize + 1);
    if (!buffer)
    {
        errorMsg("Not enough memory to read '%s'.", filepath);
        goto alloc_error;
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize)
    {
        errorMsg("Could not read file '%s'.", filepath);
        goto read_error;
    }

    buffer[bytesRead] = '\0';
    fclose(file);

    return buffer;

read_error:
    free(buffer);
alloc_error:
    fclose(file);
fail:
    return NULL;
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
