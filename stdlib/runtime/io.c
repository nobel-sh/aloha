#include "runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

char *aloha_sys_int_to_string(aloha_int value)
{
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

    if (nread > 0 && line[nread - 1] == '\n')
    {
        line[nread - 1] = '\0';
    }

    return line;
}
