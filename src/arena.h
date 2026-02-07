#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define FRAME_ARENA_SIZE (4 * 1024 * 1024)

typedef struct {
    uint8_t *buffer;
    size_t   offset;
    size_t   capacity;
} Arena;

void  arena_init(Arena *arena, size_t capacity);
void *arena_alloc(Arena *arena, size_t size);
void  arena_reset(Arena *arena);
void  arena_free(Arena *arena);

#endif // ARENA_H
