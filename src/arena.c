#include "arena.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

void arena_init(Arena *arena, size_t capacity) {
    arena->buffer   = (uint8_t *)malloc(capacity);
    arena->offset   = 0;
    arena->capacity = capacity;
    assert(arena->buffer != NULL);
}

void *arena_alloc(Arena *arena, size_t size) {
    size_t aligned_offset = (arena->offset + 15) & ~(size_t)15;
    if (aligned_offset + size > arena->capacity) {
        fprintf(stderr, "Arena out of memory: requested %zu, available %zu\n",
                size, arena->capacity - aligned_offset);
        return NULL;
    }
    void *ptr = arena->buffer + aligned_offset;
    arena->offset = aligned_offset + size;
    return ptr;
}

void arena_reset(Arena *arena) {
    arena->offset = 0;
}

void arena_free(Arena *arena) {
    free(arena->buffer);
    arena->buffer   = NULL;
    arena->offset   = 0;
    arena->capacity = 0;
}
