#include "hud.h"
#include "display.h"
#include <stdio.h>
#include <string.h>

void hud_handle_input(HUD *hud, const InputState *input) {
    if (input_was_key_pressed(input, SDL_SCANCODE_F1)) {
        hud->flags_overlay.active = !hud->flags_overlay.active;
        hud->flags_overlay.dirty = true;
    }
    if (input_was_key_pressed(input, SDL_SCANCODE_F3)) {
        hud->debug_overlay.active = !hud->debug_overlay.active;
        hud->debug_overlay.dirty = true;
    }
    if (input_was_key_pressed(input, SDL_SCANCODE_F4)) {
        hud->strip_overlay.active = !hud->strip_overlay.active;
        hud->strip_overlay.dirty = true;
    }
}

void hud_render_flags(const GlyphCache *cache, const GameFlags *flags) {
    int y = 4;
    int x = 4;
    uint32_t color = COLOR_RGB(255, 255, 0);

    char buf[128];
    snprintf(buf, sizeof(buf), "fog: %s",       flags->fog_enabled ? "ON" : "OFF");
    text_draw_string(cache, buf, x, y, color); y += cache->glyph_height + 2;

    snprintf(buf, sizeof(buf), "fly: %s",       flags->fly_mode ? "ON" : "OFF");
    text_draw_string(cache, buf, x, y, color); y += cache->glyph_height + 2;

    snprintf(buf, sizeof(buf), "wireframe: %s", flags->show_wireframe ? "ON" : "OFF");
    text_draw_string(cache, buf, x, y, color); y += cache->glyph_height + 2;

    snprintf(buf, sizeof(buf), "zbuffer: %s",   flags->show_zbuffer ? "ON" : "OFF");
    text_draw_string(cache, buf, x, y, color);
}

void hud_render_debug(const GlyphCache *cache, float fps,
                       const Camera *cam, int chunk_count) {
    int y = 4;
    int x = WINDOW_WIDTH - 30 * cache->glyph_width;
    uint32_t color = COLOR_RGB(255, 255, 255);

    char buf[128];
    snprintf(buf, sizeof(buf), "FPS: %.1f", fps);
    text_draw_string(cache, buf, x, y, color); y += cache->glyph_height + 2;

    snprintf(buf, sizeof(buf), "Pos: %.1f %.1f %.1f",
             cam->position.x, cam->position.y, cam->position.z);
    text_draw_string(cache, buf, x, y, color); y += cache->glyph_height + 2;

    snprintf(buf, sizeof(buf), "Yaw: %.1f  Pitch: %.1f",
             rad_to_deg(cam->yaw), rad_to_deg(cam->pitch));
    text_draw_string(cache, buf, x, y, color); y += cache->glyph_height + 2;

    snprintf(buf, sizeof(buf), "Chunks: %d", chunk_count);
    text_draw_string(cache, buf, x, y, color);
}

void hud_render_strips(const GlyphCache *cache, const StripPool *pool) {
    int y = WINDOW_HEIGHT - (pool->strip_count + 1) * (cache->glyph_height + 2);
    int x = 4;
    uint32_t color = COLOR_RGB(180, 255, 180);

    for (int i = 0; i < pool->strip_count; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Strip %d: %d chunks [%d-%d]",
                 i, pool->strips[i].bucket_count,
                 pool->strips[i].x_start, pool->strips[i].x_end);
        text_draw_string(cache, buf, x, y, color);
        y += cache->glyph_height + 2;
    }
}

void hud_render_zbuffer_viz(void) {
    // Find the actual depth range of rendered pixels for better contrast
    float d_min = 1.0f, d_max = 0.0f;
    for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++) {
        float d = zbuf[i];
        if (d < 1.0f) {
            if (d < d_min) d_min = d;
            if (d > d_max) d_max = d;
        }
    }

    float range = d_max - d_min;
    if (range < 1e-6f) range = 1.0f;

    for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++) {
        float d = zbuf[i];
        if (d >= 1.0f) {
            display_buffer[i] = COLOR_RGB(0, 0, 0);
        } else {
            // Remap depth to [0,1] using actual scene range, then invert
            // so closer objects are brighter
            float normalized = 1.0f - (d - d_min) / range;
            uint8_t gray = (uint8_t)(normalized * 255.0f);
            display_buffer[i] = COLOR_RGB(gray, gray, gray);
        }
    }
}
