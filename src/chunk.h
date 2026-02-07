#ifndef CHUNK_H
#define CHUNK_H

#include "math_utils.h"
#include <stdint.h>

typedef struct {
    uint32_t *pixels;
    int       width;
    int       height;
} Texture;

typedef enum {
    CHUNK_COLORED,
    CHUNK_TEXTURED,
} ChunkType;

typedef struct {
    float x, y;
    float z;
    float inv_w;  // 1/clip.w for perspective-correct interpolation
} ScreenVertex;

typedef struct {
    ChunkType    type;
    ScreenVertex verts[3];
    float        depth_sort_key;

    union {
        struct {
            uint32_t color;
        } colored;

        struct {
            Texture *texture;
            Vec2     uvs[3];
        } textured;
    };
} Chunk;

typedef struct {
    int x_min, x_max;
    int y_min, y_max;
} ScreenAABB;

void       chunks_sort_front_to_back(Chunk *chunks, int count);
ScreenAABB chunk_screen_aabb(const Chunk *chunk);

#endif // CHUNK_H
