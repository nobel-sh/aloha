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

void print(const char *str)
{
    size_t len = strlen(str);
    write(1, str, len);
}

void println(const char *str)
{
    size_t len = strlen(str);
    write(1, str, len);
    write(1, "\n", 1);
}

void printNum(double value)
{
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%g", value);
    if (len > 0)
    {
        write(1, buf, (size_t)len);
    }
}

void printlnNum(double value)
{
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%g", value);
    if (len > 0)
    {
        write(1, buf, (size_t)len);
    }
    write(1, "\n", 1);
}

char *input(void)
{
    return aloha_sys_input();
}
