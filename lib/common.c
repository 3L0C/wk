#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "common.h"

void
warnMsg(const char* fmt, ...)
{
    assert(fmt);
    static const int warnLen = strlen("[WARNING] ");

    int len = strlen(fmt) + 1; /* 1 = '\0' */
    char format[len + warnLen];
    memcpy(format, "[WARNING] ", warnLen);
    memcpy(format + warnLen, fmt, len);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, format, ap);
    va_end(ap);

    fputc((fmt[len - 2] == ':' ? ' ' : '\n'), stderr);
}

void
errorMsg(const char* fmt, ...)
{
    assert(fmt);
    static const int errorLen = strlen("[ERROR] ");

    int len = strlen(fmt) + 1; /* 1 = '\0' */
    char format[len + errorLen];
    memcpy(format, "[ERROR] ", errorLen);
    memcpy(format + errorLen, fmt, len);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, format, ap);
    va_end(ap);

    fputc((fmt[len - 2] == ':' ? ' ' : '\n'), stderr);
}
