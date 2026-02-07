#include "raster.h"
#include "display.h"
#include "flags.h"
#include <math.h>

static inline float edge_func(ScreenVertex v0, ScreenVertex v1, float px, float py) {
    return (px - v0.x) * (v1.y - v0.y) - (py - v0.y) * (v1.x - v0.x);
}

static uint32_t color_lerp(uint32_t c1, uint32_t c2, float t) {
    int r = (int)((1 - t) * ((c1 >> 16) & 0xFF) + t * ((c2 >> 16) & 0xFF));
    int g = (int)((1 - t) * ((c1 >>  8) & 0xFF) + t * ((c2 >>  8) & 0xFF));
    int b = (int)((1 - t) * ((c1)       & 0xFF) + t * ((c2)       & 0xFF));
    return COLOR_RGB(r, g, b);
}

void raster_colored_triangle(const Chunk * restrict chunk,
                              int x_min_clip, int x_max_clip) {
    ScreenVertex v0 = chunk->verts[0];
    ScreenVertex v1 = chunk->verts[1];
    ScreenVertex v2 = chunk->verts[2];
    uint32_t color = chunk->colored.color;

    int bb_x_min = maxi((int)floorf(minf(v0.x, minf(v1.x, v2.x))), x_min_clip);
    int bb_x_max = mini((int)ceilf(maxf(v0.x, maxf(v1.x, v2.x))),  x_max_clip);
    int bb_y_min = maxi((int)floorf(minf(v0.y, minf(v1.y, v2.y))), 0);
    int bb_y_max = mini((int)ceilf(maxf(v0.y, maxf(v1.y, v2.y))),  WINDOW_HEIGHT);

    float area = edge_func(v0, v1, v2.x, v2.y);
    if (fabsf(area) < 1e-6f) return;
    float inv_area = 1.0f / area;

    for (int y = bb_y_min; y < bb_y_max; y++) {
        for (int x = bb_x_min; x < bb_x_max; x++) {
            float px = x + 0.5f;
            float py = y + 0.5f;

            float w0 = edge_func(v1, v2, px, py) * inv_area;
            float w1 = edge_func(v2, v0, px, py) * inv_area;
            float w2 = edge_func(v0, v1, px, py) * inv_area;

            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            float depth = w0 * v0.z + w1 * v1.z + w2 * v2.z;

            int idx = y * WINDOW_WIDTH + x;
            if (depth < zbuf[idx]) {
                uint32_t final_color = color;
                if (g_flags.fog_enabled) {
                    float fog_factor = clampf((depth - 0.95f) / (1.0f - 0.95f), 0.0f, 1.0f);
                    final_color = color_lerp(color, COLOR_RGB(30, 30, 50), fog_factor);
                }
                zbuf[idx] = depth;
                display_buffer[idx] = final_color;
            }
        }
    }
}

void raster_textured_triangle(const Chunk * restrict chunk,
                               int x_min_clip, int x_max_clip) {
    ScreenVertex v0 = chunk->verts[0];
    ScreenVertex v1 = chunk->verts[1];
    ScreenVertex v2 = chunk->verts[2];
    Vec2 uv0 = chunk->textured.uvs[0];
    Vec2 uv1 = chunk->textured.uvs[1];
    Vec2 uv2 = chunk->textured.uvs[2];
    Texture *tex = chunk->textured.texture;

    if (!tex || !tex->pixels) return;

    int bb_x_min = maxi((int)floorf(minf(v0.x, minf(v1.x, v2.x))), x_min_clip);
    int bb_x_max = mini((int)ceilf(maxf(v0.x, maxf(v1.x, v2.x))),  x_max_clip);
    int bb_y_min = maxi((int)floorf(minf(v0.y, minf(v1.y, v2.y))), 0);
    int bb_y_max = mini((int)ceilf(maxf(v0.y, maxf(v1.y, v2.y))),  WINDOW_HEIGHT);

    float area = edge_func(v0, v1, v2.x, v2.y);
    if (fabsf(area) < 1e-6f) return;
    float inv_area = 1.0f / area;

    for (int y = bb_y_min; y < bb_y_max; y++) {
        for (int x = bb_x_min; x < bb_x_max; x++) {
            float px = x + 0.5f;
            float py = y + 0.5f;

            float w0 = edge_func(v1, v2, px, py) * inv_area;
            float w1 = edge_func(v2, v0, px, py) * inv_area;
            float w2 = edge_func(v0, v1, px, py) * inv_area;

            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            float depth = w0 * v0.z + w1 * v1.z + w2 * v2.z;

            int idx = y * WINDOW_WIDTH + x;
            if (depth >= zbuf[idx]) continue;

            float u = w0 * uv0.x + w1 * uv1.x + w2 * uv2.x;
            float v_coord = w0 * uv0.y + w1 * uv1.y + w2 * uv2.y;

            u = u - floorf(u);
            v_coord = v_coord - floorf(v_coord);

            int tex_x = (int)(u * tex->width) % tex->width;
            int tex_y = (int)(v_coord * tex->height) % tex->height;
            if (tex_x < 0) tex_x += tex->width;
            if (tex_y < 0) tex_y += tex->height;

            uint32_t texel = tex->pixels[tex_y * tex->width + tex_x];

            if (g_flags.fog_enabled) {
                float fog_factor = clampf((depth - 0.95f) / (1.0f - 0.95f), 0.0f, 1.0f);
                texel = color_lerp(texel, COLOR_RGB(30, 30, 50), fog_factor);
            }

            zbuf[idx] = depth;
            display_buffer[idx] = texel;
        }
    }
}

static void draw_line(ScreenVertex a, ScreenVertex b, uint32_t color,
                      int x_min_clip, int x_max_clip) {
    int x0 = (int)a.x, y0 = (int)a.y;
    int x1 = (int)b.x, y1 = (int)b.y;

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    int steps = dx > dy ? dx : dy;
    if (steps == 0) steps = 1;

    for (int i = 0; i <= steps; i++) {
        if (x0 >= x_min_clip && x0 < x_max_clip &&
            y0 >= 0 && y0 < WINDOW_HEIGHT) {
            float t = (steps > 0) ? (float)i / (float)steps : 0.0f;
            float depth = a.z + t * (b.z - a.z);
            int idx = y0 * WINDOW_WIDTH + x0;
            if (depth < zbuf[idx]) {
                zbuf[idx] = depth;
                display_buffer[idx] = color;
            }
        }

        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

void raster_wireframe_triangle(const Chunk * restrict chunk,
                                int x_min_clip, int x_max_clip) {
    uint32_t color = COLOR_RGB(0, 255, 0);
    draw_line(chunk->verts[0], chunk->verts[1], color, x_min_clip, x_max_clip);
    draw_line(chunk->verts[1], chunk->verts[2], color, x_min_clip, x_max_clip);
    draw_line(chunk->verts[2], chunk->verts[0], color, x_min_clip, x_max_clip);
}
