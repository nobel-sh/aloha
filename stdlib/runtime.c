/**
 * This file provides low-level system primitives that are called from
 * higher-level Aloha standard library functions.
 *
 * Note : Use 'double' for Aloha's 'number' type. TODO: change this when we have proper type support
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

double aloha_sys_write(double fd, const char *buf, double count)
{
    if (!buf || count < 0)
        return -1.0;
    return (double)write((int)fd, buf, (size_t)count);
}

double aloha_sys_read(double fd, char *buf, double count)
{
    if (!buf || count < 0)
        return -1.0;
    return (double)read((int)fd, buf, (size_t)count);
}

double aloha_sys_strlen(const char *str)
{
    if (!str)
        return 0.0;
    return (double)strlen(str);
}

double aloha_sys_num_to_str(double value, char *buf, double buf_size)
{
    if (!buf || buf_size < 2)
        return 0.0;
    int len = snprintf(buf, (size_t)buf_size, "%g", value);
    return (double)(len > 0 ? len : 0);
}

void aloha_sys_exit(double code)
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
