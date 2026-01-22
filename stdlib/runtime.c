#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

double aloha_sys_write(double fd, const char *buf, double count)
{
    return (double)write((int)fd, buf, (size_t)count);
}

double aloha_sys_read(double fd, char *buf, double count)
{
    return (double)read((int)fd, buf, (size_t)count);
}

double aloha_sys_strlen(const char *str)
{
    return (double)strlen(str);
}

void aloha_sys_exit(double code)
{
    exit((int)code);
}

double aloha_sys_num_to_str(double value, char *buf, double buf_size)
{
    int len = snprintf(buf, (size_t)buf_size, "%g", value);
    return (double)(len > 0 ? len : 0);
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
