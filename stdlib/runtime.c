/**
 * This file provides low-level system primitives that are called from
 * higher-level Aloha standard library functions.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
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
    uint8_t data[];
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

bool aloha_sys_str_eq(const char *left, const char *right)
{
    if (!left || !right)
        return left == right;

    return strcmp(left, right) == 0;
}

char *aloha_string_clone(void *arena_ptr, const char *str)
{
    if (!arena_ptr || !str)
        return NULL;

    size_t len = strlen(str);
    char *copy = aloha_arena_alloc(arena_ptr, (aloha_int)(len + 1));
    if (!copy)
        return NULL;

    memcpy(copy, str, len + 1);
    return copy;
}

char *aloha_string_concat(void *arena_ptr, const char *left, const char *right)
{
    if (!arena_ptr)
        return NULL;

    if (!left)
        left = "";
    if (!right)
        right = "";

    size_t left_len = strlen(left);
    size_t right_len = strlen(right);
    size_t total_len = left_len + right_len;
    char *result = aloha_arena_alloc(arena_ptr, (aloha_int)(total_len + 1));
    if (!result)
        return NULL;

    memcpy(result, left, left_len);
    memcpy(result + left_len, right, right_len);
    result[total_len] = '\0';
    return result;
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
    uint8_t *data;
    aloha_int len;
    aloha_int cap;
    size_t elem_size;
    aloha_arena *arena;
} aloha_vec;

static void *aloha_vec_new(void *arena_ptr, size_t elem_size)
{
    if (!arena_ptr || elem_size == 0)
        return NULL;

    aloha_vec *vec = aloha_arena_alloc(arena_ptr, sizeof(aloha_vec));
    if (!vec)
        return NULL;

    vec->data = NULL;
    vec->len = 0;
    vec->cap = 0;
    vec->elem_size = elem_size;
    vec->arena = (aloha_arena *)arena_ptr;
    return vec;
}

static int aloha_vec_grow(aloha_vec *vec)
{
    aloha_int new_cap = vec->cap == 0 ? 8 : vec->cap * 2;
    size_t byte_count = (size_t)new_cap * vec->elem_size;
    uint8_t *new_data = aloha_arena_alloc(vec->arena, (aloha_int)byte_count);
    if (!new_data)
        return 0;

    if (vec->data)
        memcpy(new_data, vec->data, (size_t)vec->len * vec->elem_size);

    vec->data = new_data;
    vec->cap = new_cap;
    return 1;
}

static void *aloha_vec_slot(aloha_vec *vec, aloha_int index)
{
    if (!vec || index < 0 || index >= vec->len)
        aloha_sys_abort();

    return vec->data + ((size_t)index * vec->elem_size);
}

static void aloha_vec_push_raw(void *vec_ptr, const void *value)
{
    aloha_vec *vec = (aloha_vec *)vec_ptr;
    if (!vec || !value)
        return;

    if (vec->len == vec->cap && !aloha_vec_grow(vec))
        return;

    memcpy(vec->data + ((size_t)vec->len * vec->elem_size), value, vec->elem_size);
    ++vec->len;
}

static void aloha_vec_get_raw(void *vec_ptr, aloha_int index, void *out)
{
    if (!out)
        aloha_sys_abort();

    aloha_vec *vec = (aloha_vec *)vec_ptr;
    memcpy(out, aloha_vec_slot(vec, index), vec->elem_size);
}

static void aloha_vec_set_raw(void *vec_ptr, aloha_int index, const void *value)
{
    if (!value)
        aloha_sys_abort();

    aloha_vec *vec = (aloha_vec *)vec_ptr;
    memcpy(aloha_vec_slot(vec, index), value, vec->elem_size);
}

static aloha_int aloha_vec_len(void *vec_ptr)
{
    aloha_vec *vec = (aloha_vec *)vec_ptr;
    return vec ? vec->len : 0;
}

void *aloha_vec_int_new(void *arena_ptr)
{
    return aloha_vec_new(arena_ptr, sizeof(aloha_int));
}

void aloha_vec_int_push(void *vec_ptr, aloha_int value)
{
    aloha_vec_push_raw(vec_ptr, &value);
}

aloha_int aloha_vec_int_len(void *vec_ptr)
{
    return aloha_vec_len(vec_ptr);
}

aloha_int aloha_vec_int_get(void *vec_ptr, aloha_int index)
{
    aloha_int value = 0;
    aloha_vec_get_raw(vec_ptr, index, &value);
    return value;
}

void aloha_vec_int_set(void *vec_ptr, aloha_int index, aloha_int value)
{
    aloha_vec_set_raw(vec_ptr, index, &value);
}

void *aloha_vec_string_new(void *arena_ptr)
{
    return aloha_vec_new(arena_ptr, sizeof(char *));
}

void aloha_vec_string_push(void *vec_ptr, char *value)
{
    aloha_vec_push_raw(vec_ptr, &value);
}

aloha_int aloha_vec_string_len(void *vec_ptr)
{
    return aloha_vec_len(vec_ptr);
}

char *aloha_vec_string_get(void *vec_ptr, aloha_int index)
{
    char *value = NULL;
    aloha_vec_get_raw(vec_ptr, index, &value);
    return value;
}

void aloha_vec_string_set(void *vec_ptr, aloha_int index, char *value)
{
    aloha_vec_set_raw(vec_ptr, index, &value);
}
