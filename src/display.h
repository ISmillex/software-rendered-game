#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define DISPLAY_WIDTH  WINDOW_WIDTH
#define DISPLAY_HEIGHT WINDOW_HEIGHT

extern uint32_t display_buffer[];
extern float    zbuf[];

#define COLOR_ARGB(a, r, g, b) \
    (((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | \
     ((uint32_t)(g) << 8)  |  (uint32_t)(b))

#define COLOR_RGB(r, g, b)  COLOR_ARGB(0xFF, r, g, b)

bool display_init(void);
void display_clear(uint32_t background_color);
void display_present(void);
void display_destroy(void);

static inline void display_set_pixel(int x, int y, float depth, uint32_t color) {
    if (x < 0 || x >= WINDOW_WIDTH || y < 0 || y >= WINDOW_HEIGHT) return;
    int idx = y * WINDOW_WIDTH + x;
    if (depth < zbuf[idx]) {
        zbuf[idx] = depth;
        display_buffer[idx] = color;
    }
}

static inline void display_set_pixel_unchecked(int x, int y, float depth, uint32_t color) {
    int idx = y * WINDOW_WIDTH + x;
    if (depth < zbuf[idx]) {
        zbuf[idx] = depth;
        display_buffer[idx] = color;
    }
}

#endif // DISPLAY_H
