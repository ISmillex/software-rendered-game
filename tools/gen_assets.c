// Generates procedural game assets (textures + models)
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void write_bmp(const char *path, uint8_t *pixels, int w, int h) {
    int row_size = ((w * 3 + 3) / 4) * 4;
    int data_size = row_size * h;
    int file_size = 54 + data_size;

    uint8_t header[54];
    memset(header, 0, 54);
    header[0] = 'B'; header[1] = 'M';
    *(int*)&header[2] = file_size;
    *(int*)&header[10] = 54;
    *(int*)&header[14] = 40;
    *(int*)&header[18] = w;
    *(int*)&header[22] = h;
    *(short*)&header[26] = 1;
    *(short*)&header[28] = 24;
    *(int*)&header[34] = data_size;

    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "Failed to create %s\n", path); return; }
    fwrite(header, 1, 54, f);

    uint8_t *row = calloc(row_size, 1);
    for (int y = h - 1; y >= 0; y--) {
        for (int x = 0; x < w; x++) {
            int src = (y * w + x) * 3;
            row[x * 3 + 0] = pixels[src + 2]; // B
            row[x * 3 + 1] = pixels[src + 1]; // G
            row[x * 3 + 2] = pixels[src + 0]; // R
        }
        fwrite(row, 1, row_size, f);
    }
    free(row);
    fclose(f);
    printf("Created %s (%dx%d)\n", path, w, h);
}

static void gen_crate_texture(const char *path) {
    int w = 64, h = 64;
    uint8_t *pixels = malloc(w * h * 3);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 3;
            // Base wood color
            uint8_t r = 160, g = 110, b = 60;

            // Wood grain variation
            int grain = ((x * 7 + y * 3) % 17);
            r = (uint8_t)(r + grain * 2 - 17);
            g = (uint8_t)(g + grain - 8);

            // Crate border lines
            int border = 4;
            int mid = w / 2;
            if (x < border || x >= w - border || y < border || y >= h - border) {
                r = 100; g = 70; b = 35;
            }
            // Cross planks
            if ((x >= mid - 1 && x <= mid + 1) || (y >= mid - 1 && y <= mid + 1)) {
                r = 110; g = 75; b = 38;
            }
            // Corner nails
            if ((x >= border && x <= border + 2 && y >= border && y <= border + 2) ||
                (x >= w - border - 3 && x <= w - border - 1 && y >= border && y <= border + 2) ||
                (x >= border && x <= border + 2 && y >= h - border - 3 && y <= h - border - 1) ||
                (x >= w - border - 3 && x <= w - border - 1 && y >= h - border - 3 && y <= h - border - 1)) {
                r = 180; g = 180; b = 170;
            }

            pixels[idx + 0] = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
        }
    }

    write_bmp(path, pixels, w, h);
    free(pixels);
}

static void gen_floor_texture(const char *path) {
    int w = 64, h = 64;
    uint8_t *pixels = malloc(w * h * 3);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 3;

            // Tile grid: 2x2 tiles, each 32px (more visible at distance)
            int tile_x = x / 32;
            int tile_y = y / 32;
            int lx = x % 32;
            int ly = y % 32;

            // Grout lines (2px borders)
            if (lx < 2 || ly < 2) {
                pixels[idx + 0] = 80;
                pixels[idx + 1] = 80;
                pixels[idx + 2] = 78;
                continue;
            }

            // Base stone gray with per-tile tint variation
            int tint = (tile_x * 3 + tile_y * 7) % 5;
            uint8_t r = (uint8_t)(140 + tint * 2 - 4);
            uint8_t g = (uint8_t)(138 + tint - 2);
            uint8_t b = (uint8_t)(130 + tint - 2);

            // Per-pixel noise for roughness
            unsigned int hash = ((unsigned int)(x * 2654435761u) ^ (unsigned int)(y * 2246822519u));
            int noise = (int)(hash % 17) - 8;
            r = (uint8_t)(r + noise);
            g = (uint8_t)(g + noise);
            b = (uint8_t)(b + noise);

            pixels[idx + 0] = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
        }
    }

    write_bmp(path, pixels, w, h);
    free(pixels);
}

static void gen_wall_texture(const char *path) {
    int w = 64, h = 64;
    uint8_t *pixels = malloc(w * h * 3);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 3;

            // Brick layout: 8px tall rows, 16px wide bricks
            int row = y / 8;
            int offset = (row % 2) * 8;  // stagger odd rows
            int bx = (x + offset) % 64;
            int ly = y % 8;
            int lx = bx % 16;

            // Mortar lines
            if (ly < 2 || lx < 2) {
                pixels[idx + 0] = 180;
                pixels[idx + 1] = 175;
                pixels[idx + 2] = 165;
                continue;
            }

            // Brick body: warm red-brown
            int brick_col = bx / 16;
            int brick_var = (brick_col * 13 + row * 7) % 11;
            uint8_t r = (uint8_t)(155 + brick_var - 5);
            uint8_t g = (uint8_t)(75 + brick_var / 2 - 2);
            uint8_t b = (uint8_t)(55 + brick_var / 3 - 1);

            // Per-pixel noise
            unsigned int hash = ((unsigned int)(x * 2654435761u) ^ (unsigned int)(y * 2246822519u));
            int noise = (int)(hash % 13) - 6;
            r = (uint8_t)(r + noise);
            g = (uint8_t)(g + noise / 2);
            b = (uint8_t)(b + noise / 3);

            pixels[idx + 0] = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
        }
    }

    write_bmp(path, pixels, w, h);
    free(pixels);
}

static void write_obj_data(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) { fprintf(stderr, "Failed to create %s\n", path); return; }
    fputs(content, f);
    fclose(f);
    printf("Created %s\n", path);
}

static void gen_floor_obj(const char *path) {
    // Generate a grid of quads for better strip distribution and culling
    // Each quad is 1x1 in model space, tiled NxN
    // The whole floor spans -0.5 to 0.5 (scaled by scene to 40 units)
    int grid = 8;  // 8x8 grid of quads
    float cell = 1.0f / grid;

    FILE *f = fopen(path, "w");
    if (!f) { fprintf(stderr, "Failed to create %s\n", path); return; }

    fprintf(f, "# Floor grid (%dx%d quads)\n", grid, grid);

    // Vertices: (grid+1) x (grid+1)
    for (int z = 0; z <= grid; z++) {
        for (int x = 0; x <= grid; x++) {
            fprintf(f, "v %.4f 0.0 %.4f\n",
                    -0.5f + x * cell, 0.5f - z * cell);
        }
    }

    // UVs: 1 texture repeat per quad
    fprintf(f, "vt 0.0 0.0\n");  // 1
    fprintf(f, "vt 1.0 0.0\n");  // 2
    fprintf(f, "vt 1.0 1.0\n");  // 3
    fprintf(f, "vt 0.0 1.0\n");  // 4

    // Faces
    int row = grid + 1;
    for (int z = 0; z < grid; z++) {
        for (int x = 0; x < grid; x++) {
            int v0 = z * row + x + 1;        // OBJ is 1-indexed
            int v1 = v0 + 1;
            int v2 = v0 + row + 1;
            int v3 = v0 + row;
            fprintf(f, "f %d/1 %d/2 %d/3\n", v0, v1, v2);
            fprintf(f, "f %d/1 %d/3 %d/4\n", v0, v2, v3);
        }
    }

    fclose(f);
    printf("Created %s (%dx%d grid)\n", path, grid, grid);
}

static void gen_wall_obj(const char *path) {
    write_obj_data(path,
        "# Wall segment: 1.0 x 1.0 x 0.1\n"
        "v -0.5 -0.5  0.05\n"
        "v  0.5 -0.5  0.05\n"
        "v  0.5  0.5  0.05\n"
        "v -0.5  0.5  0.05\n"
        "v -0.5 -0.5 -0.05\n"
        "v  0.5 -0.5 -0.05\n"
        "v  0.5  0.5 -0.05\n"
        "v -0.5  0.5 -0.05\n"
        "\n"
        "# Large face UVs (front/back): tile 12x across, 2x up\n"
        "vt  0.0 0.0\n"  /* 1 */
        "vt 12.0 0.0\n"  /* 2 */
        "vt 12.0 2.0\n"  /* 3 */
        "vt  0.0 2.0\n"  /* 4 */
        "# Side face UVs (left/right): narrow\n"
        "vt 0.0 0.0\n"   /* 5 */
        "vt 0.4 0.0\n"   /* 6 */
        "vt 0.4 2.0\n"   /* 7 */
        "vt 0.0 2.0\n"   /* 8 */
        "# Cap UVs (top/bottom)\n"
        "vt  0.0 0.0\n"  /* 9 */
        "vt 12.0 0.0\n"  /* 10 */
        "vt 12.0 0.4\n"  /* 11 */
        "vt  0.0 0.4\n"  /* 12 */
        "\n"
        "# Front face\n"
        "f 1/1 2/2 3/3\n"
        "f 1/1 3/3 4/4\n"
        "# Back face\n"
        "f 6/1 5/2 8/3\n"
        "f 6/1 8/3 7/4\n"
        "# Right face\n"
        "f 2/5 6/6 7/7\n"
        "f 2/5 7/7 3/8\n"
        "# Left face\n"
        "f 5/5 1/6 4/7\n"
        "f 5/5 4/7 8/8\n"
        "# Top face\n"
        "f 4/9 3/10 7/11\n"
        "f 4/9 7/11 8/12\n"
        "# Bottom face\n"
        "f 5/9 6/10 2/11\n"
        "f 5/9 2/11 1/12\n"
    );
}

static void gen_sphere_obj(const char *path, int slices, int stacks) {
    // Unit sphere: radius 0.5, centered at origin (diameter = 1.0, like cube)
    FILE *f = fopen(path, "w");
    if (!f) { fprintf(stderr, "Failed to create %s\n", path); return; }

    fprintf(f, "# Sphere (%d slices x %d stacks)\n", slices, stacks);

    // Vertices and UVs: (stacks+1) rows of (slices+1) columns
    for (int j = 0; j <= stacks; j++) {
        float phi = M_PI * (float)j / (float)stacks;  // 0 to PI (top to bottom)
        float sp = sinf(phi);
        float cp = cosf(phi);
        for (int i = 0; i <= slices; i++) {
            float theta = 2.0f * M_PI * (float)i / (float)slices;  // 0 to 2PI
            float st = sinf(theta);
            float ct = cosf(theta);
            float x = sp * ct * 0.5f;
            float y = cp * 0.5f;
            float z = sp * st * 0.5f;
            fprintf(f, "v %.6f %.6f %.6f\n", x, y, z);
        }
    }

    for (int j = 0; j <= stacks; j++) {
        for (int i = 0; i <= slices; i++) {
            float u = (float)i / (float)slices;
            float v = (float)j / (float)stacks;
            fprintf(f, "vt %.6f %.6f\n", u, 1.0f - v);
        }
    }

    // Faces: each grid cell = 2 triangles
    // At the poles, only emit one triangle (fan) to avoid degenerates
    int row = slices + 1;
    for (int j = 0; j < stacks; j++) {
        for (int i = 0; i < slices; i++) {
            int v0 = j * row + i + 1;       // OBJ 1-indexed
            int v1 = v0 + 1;
            int v2 = v0 + row;
            int v3 = v2 + 1;

            if (j == 0) {
                // Top pole: fan triangle from pole to first ring
                fprintf(f, "f %d/%d %d/%d %d/%d\n", v0, v0, v3, v3, v2, v2);
            } else if (j == stacks - 1) {
                // Bottom pole: fan triangle from ring to bottom pole
                fprintf(f, "f %d/%d %d/%d %d/%d\n", v0, v0, v1, v1, v2, v2);
            } else {
                // Normal quad = 2 triangles
                fprintf(f, "f %d/%d %d/%d %d/%d\n", v0, v0, v3, v3, v2, v2);
                fprintf(f, "f %d/%d %d/%d %d/%d\n", v0, v0, v1, v1, v3, v3);
            }
        }
    }

    fclose(f);
    printf("Created %s (%d slices x %d stacks)\n", path, slices, stacks);
}

static void gen_stone_texture(const char *path) {
    int w = 32, h = 32;
    uint8_t *pixels = malloc(w * h * 3);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 3;

            // Base gray
            uint8_t r = 120, g = 118, b = 115;

            // Strong per-pixel noise for rough rock look
            unsigned int hash = ((unsigned int)(x * 2654435761u) ^ (unsigned int)(y * 2246822519u));
            int noise = (int)(hash % 25) - 12;
            r = (uint8_t)(r + noise);
            g = (uint8_t)(g + noise);
            b = (uint8_t)(b + noise - 2);

            // Darker speckles
            unsigned int speckle = ((unsigned int)((x + 7) * 1103515245u) ^ (unsigned int)((y + 13) * 12345u));
            if ((speckle % 11) < 2) {
                r = (uint8_t)(r * 0.7f);
                g = (uint8_t)(g * 0.7f);
                b = (uint8_t)(b * 0.7f);
            }

            pixels[idx + 0] = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
        }
    }

    write_bmp(path, pixels, w, h);
    free(pixels);
}

static void gen_ball_texture(const char *path) {
    int w = 32, h = 32;
    uint8_t *pixels = malloc(w * h * 3);

    float cx = w / 2.0f, cy = h / 2.0f;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 3;

            // Bright red-orange base
            uint8_t r = 200, g = 60, b = 40;

            // White stripe band (horizontal in UV space)
            if (y >= 14 && y <= 17) {
                r = 240; g = 240; b = 240;
            }

            // Specular highlight in upper-left area
            float dx = (float)x - cx * 0.6f;
            float dy = (float)y - cy * 0.4f;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < 6.0f) {
                float t = 1.0f - dist / 6.0f;
                r = (uint8_t)(r + (255 - r) * t * 0.7f);
                g = (uint8_t)(g + (255 - g) * t * 0.7f);
                b = (uint8_t)(b + (255 - b) * t * 0.7f);
            }

            // Subtle per-pixel noise
            unsigned int hash = ((unsigned int)(x * 2654435761u) ^ (unsigned int)(y * 2246822519u));
            int noise = (int)(hash % 9) - 4;
            r = (uint8_t)((int)r + noise < 0 ? 0 : (int)r + noise > 255 ? 255 : (int)r + noise);
            g = (uint8_t)((int)g + noise < 0 ? 0 : (int)g + noise > 255 ? 255 : (int)g + noise);
            b = (uint8_t)((int)b + noise < 0 ? 0 : (int)b + noise > 255 ? 255 : (int)b + noise);

            pixels[idx + 0] = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
        }
    }

    write_bmp(path, pixels, w, h);
    free(pixels);
}

int main(void) {
    gen_crate_texture("assets/textures/crate.bmp");
    gen_floor_texture("assets/textures/floor.bmp");
    gen_wall_texture("assets/textures/wall.bmp");
    gen_stone_texture("assets/textures/stone.bmp");
    gen_ball_texture("assets/textures/ball.bmp");
    gen_floor_obj("assets/models/floor.obj");
    gen_wall_obj("assets/models/wall.obj");
    gen_sphere_obj("assets/models/sphere_lo.obj", 8, 6);
    gen_sphere_obj("assets/models/sphere_hi.obj", 16, 12);
    printf("Assets generated successfully!\n");
    return 0;
}
