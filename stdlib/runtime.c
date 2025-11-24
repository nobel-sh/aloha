#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double runtime_write(double fd, const char *buf, double count)
{
    int fd_int = (int)fd;
    size_t count_size = (size_t)count;
    if (fd_int == 1)
    {
        return (double)fwrite(buf, 1, count_size, stdout);
    }
    else if (fd_int == 2)
    {
        return (double)fwrite(buf, 1, count_size, stderr);
    }
    return -1.0;
}

double runtime_read(double fd, char *buf, double count)
{
    int fd_int = (int)fd;
    size_t count_size = (size_t)count;
    if (fd_int == 0)
    {
        return (double)fread(buf, 1, count_size, stdin);
    }
    return -1.0;
}

void runtime_flush(double fd)
{
    int fd_int = (int)fd;
    if (fd_int == 1)
    {
        fflush(stdout);
    }
    else if (fd_int == 2)
    {
        fflush(stderr);
    }
}

void *runtime_malloc(double size)
{
    return malloc((size_t)size);
}

void runtime_free(void *ptr)
{
    free(ptr);
}

double runtime_strlen(const char *str)
{
    return (double)strlen(str);
}

double runtime_memcmp(const char *a, const char *b, double n)
{
    return (double)memcmp(a, b, (size_t)n);
}

void runtime_memcpy(char *dst, const char *src, double n)
{
    memcpy(dst, src, (size_t)n);
}

void runtime_exit(double code)
{
    exit((int)code);
}

void runtime_abort(void)
{
    abort();
}

void print(const char *str)
{
    runtime_write(1, str, (double)strlen(str));
}

void println(const char *str)
{
    runtime_write(1, str, (double)strlen(str));
    runtime_write(1, "\n", 1.0);
}

void printNum(double value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%g", value);
    runtime_write(1, buf, (double)strlen(buf));
}

void printlnNum(double value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%g", value);
    runtime_write(1, buf, (double)strlen(buf));
    runtime_write(1, "\n", 1.0);
}

char *input(void)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t read = getline(&line, &len, stdin);

    if (read == -1)
    {
        free(line);
        return NULL;
    }

    if (read > 0 && line[read - 1] == '\n')
    {
        line[read - 1] = '\0';
    }

    return line;
}
