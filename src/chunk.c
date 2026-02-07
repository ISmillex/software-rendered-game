#include "chunk.h"
#include <stdlib.h>
#include <math.h>

static int chunk_compare(const void *a, const void *b) {
    float da = ((const Chunk *)a)->depth_sort_key;
    float db = ((const Chunk *)b)->depth_sort_key;
    if (da < db) return -1;
    if (da > db) return  1;
    return 0;
}

void chunks_sort_front_to_back(Chunk *chunks, int count) {
    qsort(chunks, count, sizeof(Chunk), chunk_compare);
}

ScreenAABB chunk_screen_aabb(const Chunk *chunk) {
    ScreenAABB bb;
    bb.x_min = (int)floorf(minf(chunk->verts[0].x,
                                minf(chunk->verts[1].x, chunk->verts[2].x)));
    bb.x_max = (int)ceilf(maxf(chunk->verts[0].x,
                                maxf(chunk->verts[1].x, chunk->verts[2].x)));
    bb.y_min = (int)floorf(minf(chunk->verts[0].y,
                                minf(chunk->verts[1].y, chunk->verts[2].y)));
    bb.y_max = (int)ceilf(maxf(chunk->verts[0].y,
                                maxf(chunk->verts[1].y, chunk->verts[2].y)));
    return bb;
}
