// Generates a 64x64 crate texture as BMP
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

int main(void) {
    gen_crate_texture("assets/textures/crate.bmp");
    printf("Assets generated successfully!\n");
    return 0;
}
