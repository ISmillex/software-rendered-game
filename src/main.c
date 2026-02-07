#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "math_utils.h"
#include "arena.h"
#include "display.h"
#include "input.h"
#include "camera.h"
#include "chunk.h"
#include "scene.h"
#include "raster.h"
#include "strip.h"
#include "text.h"
#include "console.h"
#include "flags.h"
#include "hud.h"

static Camera     camera;
static Scene      scene;
static Console    console;
static HUD        hud;
static GlyphCache glyph_cache;
static Arena      frame_arena;
static StripPool  strip_pool;
static InputState input_state;

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    // 1. Display (SDL)
    if (!display_init()) {
        fprintf(stderr, "Failed to initialize display\n");
        return 1;
    }

    // 2. Input
    input_init();

    // 3. Camera
    camera_init(&camera);

    // 4. Glyph cache (try file first, fallback to generated)
    if (!glyph_cache_init(&glyph_cache, "assets/font/font.bmp", 8, 16)) {
        fprintf(stderr, "Warning: Could not init glyph cache\n");
    }

    // 5. Console
    console_init(&console);

    // 6. Game flags + register console commands
    flags_register_commands(&console);

    // 7. Arena allocator
    arena_init(&frame_arena, FRAME_ARENA_SIZE);

    // 8. Thread pool
    int num_cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cores < 1) num_cores = 4;
    if (num_cores > 16) num_cores = 16;
    strip_pool_init(&strip_pool, num_cores);

    // 9. Scene - load models and place objects
    scene_init(&scene);
    Model *cube_model = scene_load_model(&scene, "assets/models/cube.obj",
                                          "assets/textures/crate.bmp");
    if (cube_model) {
        // Central cube
        scene_add_object(&scene, cube_model,
                         vec3(0, 0, 0), vec3(0, 0, 0), 1.0f);
        // Additional cubes in a grid
        for (int x = -3; x <= 3; x += 3) {
            for (int z = -3; z <= 3; z += 3) {
                if (x == 0 && z == 0) continue;
                int idx = scene_add_object(&scene, cube_model,
                                 vec3((float)x, 0, (float)z),
                                 vec3(0, 0, 0), 0.7f);
                if (idx >= 0) {
                    scene.objects[idx].anim_bounce = true;
                    scene.objects[idx].anim_speed = 1.0f + (float)(abs(x) + abs(z)) * 0.2f;
                    scene.objects[idx].anim_amplitude = 0.3f;
                }
            }
        }
    } else {
        fprintf(stderr, "Warning: Could not load cube model\n");
    }

    // 10. HUD
    memset(&hud, 0, sizeof(hud));
    hud.debug_overlay.active = true; // show FPS by default

    console_print(&console, "Software Renderer started", COLOR_RGB(100, 255, 100));
    console_print(&console, "Type 'help' for commands", COLOR_RGB(200, 200, 200));

    // --- Main Loop ---
    bool running = true;
    Uint64 last_time = SDL_GetPerformanceCounter();
    float fps = 0.0f;
    float fps_accum = 0.0f;
    int fps_frame_count = 0;

    while (running) {
        // --- TIMING ---
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last_time) / (float)SDL_GetPerformanceFrequency();
        last_time = now;
        // Clamp dt to avoid spiral of death
        if (dt > 0.1f) dt = 0.1f;

        fps_accum += dt;
        fps_frame_count++;
        if (fps_accum >= 0.5f) {
            fps = fps_frame_count / fps_accum;
            fps_accum = 0.0f;
            fps_frame_count = 0;
        }

        // --- 1. INPUT ---
        input_poll(&input_state);
        if (input_state.quit_requested) {
            running = false;
            break;
        }

        // Console toggle (always active)
        if (input_was_key_pressed(&input_state, SDL_SCANCODE_GRAVE)) {
            console.open = !console.open;
            // Discard text input from the toggle frame so the backtick
            // character (or dead-key variant) doesn't leak into the console
            input_state.text_input_len = 0;
            input_state.text_input[0] = '\0';
        }

        // Escape to quit
        if (input_was_key_pressed(&input_state, SDL_SCANCODE_ESCAPE)) {
            running = false;
            break;
        }

        // Route input
        if (console.open) {
            console_handle_input(&console, &input_state);
        } else {
            camera_handle_input(&camera, &input_state, dt);
            hud_handle_input(&hud, &input_state);
        }

        // Sync fly_mode flag to camera
        camera.fly_mode = g_flags.fly_mode;

        // --- 2. UPDATE ---
        scene_update(&scene, dt);

        // --- 3. CHUNK GENERATION ---
        arena_reset(&frame_arena);
        int max_chunks = 16384;
        Chunk *chunks = arena_alloc(&frame_arena, max_chunks * sizeof(Chunk));
        int chunk_count = 0;

        if (chunks) {
            float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
            Mat4 vp = camera_vp_matrix(&camera, aspect);
            scene_generate_chunks(&scene, &vp, &frame_arena,
                                  chunks, &chunk_count, max_chunks);

            // Sort front-to-back
            chunks_sort_front_to_back(chunks, chunk_count);

            // --- 4. STRIP DISTRIBUTION ---
            strip_pool_distribute(&strip_pool, chunks, chunk_count);
        }

        // --- 5. PARALLEL RENDER ---
        display_clear(COLOR_RGB(30, 30, 50));
        if (chunks) {
            strip_pool_render(&strip_pool);
        }

        // --- 6. Z-BUFFER VISUALIZATION ---
        if (g_flags.show_zbuffer) {
            hud_render_zbuffer_viz();
        }

        // --- 7. OVERLAY COMPOSITING ---
        if (hud.flags_overlay.active) {
            hud_render_flags(&glyph_cache, &g_flags);
        }
        if (hud.debug_overlay.active) {
            hud_render_debug(&glyph_cache, fps, &camera, chunk_count);
        }
        if (hud.strip_overlay.active) {
            hud_render_strips(&glyph_cache, &strip_pool);
        }
        if (console.open) {
            console_render(&console, &glyph_cache);
        }

        // --- 8. PRESENT ---
        display_present();
    }

    // Cleanup
    strip_pool_destroy(&strip_pool);
    arena_free(&frame_arena);
    scene_destroy(&scene);
    display_destroy();
    SDL_Quit();

    return 0;
}
