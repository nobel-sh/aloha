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

typedef struct aloha_arena_block
{
    struct aloha_arena_block *next;
    size_t capacity;
    size_t used;
    unsigned char data[];
} aloha_arena_block;

typedef struct aloha_arena
{
    aloha_arena_block *blocks;
} aloha_arena;

static size_t aloha_align_up(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

static aloha_arena_block *aloha_arena_new_block(size_t capacity)
{
    aloha_arena_block *block = (aloha_arena_block *)malloc(sizeof(aloha_arena_block) + capacity);
    if (!block)
        return NULL;
    block->next = NULL;
    block->capacity = capacity;
    block->used = 0;
    return block;
}

void *aloha_arena_new(void)
{
    aloha_arena *arena = (aloha_arena *)malloc(sizeof(aloha_arena));
    if (!arena)
        return NULL;
    arena->blocks = NULL;
    return arena;
}

void *aloha_arena_alloc(void *arena_ptr, aloha_int size)
{
    if (!arena_ptr || size <= 0)
        return NULL;

    aloha_arena *arena = (aloha_arena *)arena_ptr;
    size_t requested = aloha_align_up((size_t)size, 8);
    size_t default_capacity = 4096;
    size_t capacity = requested > default_capacity ? requested : default_capacity;

    aloha_arena_block *block = arena->blocks;
    if (!block || block->used + requested > block->capacity)
    {
        aloha_arena_block *new_block = aloha_arena_new_block(capacity);
        if (!new_block)
            return NULL;
        new_block->next = arena->blocks;
        arena->blocks = new_block;
        block = new_block;
    }

    void *ptr = block->data + block->used;
    block->used += requested;
    return ptr;
}

void aloha_arena_free_all(void *arena_ptr)
{
    if (!arena_ptr)
        return;

    aloha_arena *arena = (aloha_arena *)arena_ptr;
    aloha_arena_block *block = arena->blocks;
    while (block)
    {
        aloha_arena_block *next = block->next;
        free(block);
        block = next;
    }
    free(arena);
}

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

typedef struct
{
    aloha_int *data;
    aloha_int len;
    aloha_int cap;
    aloha_arena *arena;
} aloha_vec_int;

void *aloha_vec_int_new(void *arena_ptr)
{
    if (!arena_ptr)
        return NULL;

    aloha_vec_int *vec = aloha_arena_alloc(arena_ptr, sizeof(aloha_vec_int));
    if (!vec)
        return NULL;

    vec->data = NULL;
    vec->arena = (aloha_arena *)arena_ptr;
    vec->cap = 0;
    vec->len = 0;
    return vec;
}

static int aloha_vec_int_grow(aloha_vec_int *vec_ptr)
{
    aloha_int new_cap = vec_ptr->cap == 0 ? 8 : vec_ptr->cap * 2;
    aloha_int *new_data = aloha_arena_alloc(vec_ptr->arena, new_cap * (aloha_int)sizeof(aloha_int));

    if (!new_data)
        return 0;

    for (aloha_int i = 0; i < vec_ptr->len; ++i)
        new_data[i] = vec_ptr->data[i];

    vec_ptr->data = new_data;
    vec_ptr->cap = new_cap;
    return 1;
}

void aloha_vec_int_push(void *vec_ptr, aloha_int value)
{
    aloha_vec_int *vec = (aloha_vec_int *)vec_ptr;
    if (!vec)
        return;

    if (vec->len == vec->cap && !aloha_vec_int_grow(vec))
        return;

    vec->data[vec->len] = value;
    ++vec->len;
}

aloha_int aloha_vec_int_len(void *vec_ptr)
{
    aloha_vec_int *vec = (aloha_vec_int *)vec_ptr;
    return vec ? vec->len : 0;
}

aloha_int aloha_vec_int_get(void *vec_ptr, aloha_int index)
{
    aloha_vec_int *vec = (aloha_vec_int *)vec_ptr;
    if (!vec || index < 0 || index >= vec->len)
        aloha_sys_abort();

    return vec->data[index];
}

void aloha_vec_int_set(void *vec_ptr, aloha_int index, aloha_int value)
{
    aloha_vec_int *vec = (aloha_vec_int *)vec_ptr;
    if (!vec || index < 0 || index >= vec->len)
        aloha_sys_abort();
    vec->data[index] = value;
}
