#ifndef RASTER_H
#define RASTER_H

#include "chunk.h"

void raster_colored_triangle(const Chunk *chunk, int x_min_clip, int x_max_clip);
void raster_textured_triangle(const Chunk *chunk, int x_min_clip, int x_max_clip);
void raster_wireframe_triangle(const Chunk *chunk, int x_min_clip, int x_max_clip);

#endif // RASTER_H
