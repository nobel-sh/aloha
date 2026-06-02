#include "runtime.h"

#include <stdlib.h>

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
