#include "runtime.h"

#include <string.h>

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

aloha_int aloha_string_char_at(const char *str, aloha_int index)
{
    if (!str || index < 0)
        aloha_sys_abort();

    size_t str_len = strlen(str);
    if ((size_t)index >= str_len)
        aloha_sys_abort();

    return (aloha_int)(unsigned char)str[index];
}

char *aloha_string_slice(void *arena_ptr, const char *str, aloha_int start, aloha_int len)
{
    if (!arena_ptr || !str || start < 0 || len < 0)
        aloha_sys_abort();

    size_t str_len = strlen(str);
    size_t slice_start = (size_t)start;
    size_t slice_len = (size_t)len;
    if (slice_start > str_len || slice_len > str_len - slice_start)
        aloha_sys_abort();

    char *result = aloha_arena_alloc(arena_ptr, (aloha_int)(slice_len + 1));
    if (!result)
        return NULL;

    memcpy(result, str + slice_start, slice_len);
    result[slice_len] = '\0';
    return result;
}
