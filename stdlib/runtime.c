/**
 * This file provides low-level system primitives that are called from
 * higher-level Aloha standard library functions.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// aliases for Aloha types
typedef int64_t aloha_int;
typedef double aloha_float;

aloha_int aloha_sys_write(aloha_int fd, const char *buf, aloha_int count)
{
    if (!buf || count < 0)
        return -1;
    ssize_t result = write((int)fd, buf, (size_t)count);
    return (aloha_int)result;
}

aloha_int aloha_sys_read(aloha_int fd, char *buf, aloha_int count)
{
    if (!buf || count < 0)
        return -1;
    ssize_t result = read((int)fd, buf, (size_t)count);
    return (aloha_int)result;
}

aloha_int aloha_sys_strlen(const char *str)
{
    if (!str)
        return 0;
    return (aloha_int)strlen(str);
}

char *aloha_sys_int_to_string(aloha_int value)
{
    // 24 bytes for int64_t
    char *buf = (char *)malloc(24);
    if (!buf)
        return NULL;

    int len = snprintf(buf, 24, "%ld", value);
    if (len < 0)
    {
        free(buf);
        return NULL;
    }

    return buf;
}

char *aloha_sys_float_to_string(aloha_float value)
{
    // 32 bytes for a double
    char *buf = (char *)malloc(32);
    if (!buf)
        return NULL;

    int len = snprintf(buf, 32, "%g", value);
    if (len < 0)
    {
        free(buf);
        return NULL;
    }

    return buf;
}

void aloha_sys_exit(aloha_int code)
{
    exit((int)code);
}

void aloha_sys_abort(void)
{
    abort();
}

char *aloha_sys_input(void)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t nread = getline(&line, &len, stdin);

    if (nread == -1)
    {
        free(line);
        return NULL;
    }

    // Remove trailing newline if present
    if (nread > 0 && line[nread - 1] == '\n')
    {
        line[nread - 1] = '\0';
    }

    return line;
}
