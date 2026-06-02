#include "runtime.h"

#include <string.h>

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
