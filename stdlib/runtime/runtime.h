#ifndef ALOHA_STDLIB_RUNTIME_H_
#define ALOHA_STDLIB_RUNTIME_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

void *aloha_arena_new(void);
void *aloha_arena_alloc(void *arena_ptr, aloha_int size);
void aloha_arena_free_all(void *arena_ptr);

aloha_int aloha_sys_write(aloha_int fd, const char *buf, aloha_int count);
aloha_int aloha_sys_read(aloha_int fd, char *buf, aloha_int count);
aloha_int aloha_sys_strlen(const char *str);
bool aloha_sys_str_eq(const char *left, const char *right);
char *aloha_sys_int_to_string(aloha_int value);
char *aloha_sys_float_to_string(aloha_float value);
void aloha_sys_exit(aloha_int code);
void aloha_sys_abort(void);
char *aloha_sys_input(void);

char *aloha_string_clone(void *arena_ptr, const char *str);
char *aloha_string_concat(void *arena_ptr, const char *left, const char *right);

void *aloha_vec_int_new(void *arena_ptr);
void aloha_vec_int_push(void *vec_ptr, aloha_int value);
aloha_int aloha_vec_int_len(void *vec_ptr);
aloha_int aloha_vec_int_get(void *vec_ptr, aloha_int index);
void aloha_vec_int_set(void *vec_ptr, aloha_int index, aloha_int value);

void *aloha_vec_string_new(void *arena_ptr);
void aloha_vec_string_push(void *vec_ptr, char *value);
aloha_int aloha_vec_string_len(void *vec_ptr);
char *aloha_vec_string_get(void *vec_ptr, aloha_int index);
void aloha_vec_string_set(void *vec_ptr, aloha_int index, char *value);

#endif // ALOHA_STDLIB_RUNTIME_H_
