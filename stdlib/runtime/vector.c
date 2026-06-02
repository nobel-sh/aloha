#include "runtime.h"

#include <string.h>

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
