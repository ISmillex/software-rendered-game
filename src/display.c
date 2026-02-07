#include "display.h"
#include <SDL2/SDL.h>

uint32_t display_buffer[WINDOW_WIDTH * WINDOW_HEIGHT];
float    zbuf[WINDOW_WIDTH * WINDOW_HEIGHT];

static SDL_Window   *window;
static SDL_Renderer *sdl_renderer;
static SDL_Texture  *framebuffer_texture;

bool display_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(
        "Software Renderer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        0
    );
    if (!window) return false;

    sdl_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!sdl_renderer) return false;

    framebuffer_texture = SDL_CreateTexture(
        sdl_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WINDOW_WIDTH, WINDOW_HEIGHT
    );
    if (!framebuffer_texture) return false;

    return true;
}

void display_clear(uint32_t background_color) {
    for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++) {
        display_buffer[i] = background_color;
        zbuf[i] = 1.0f;
    }
}

void display_present(void) {
    SDL_UpdateTexture(
        framebuffer_texture,
        NULL,
        display_buffer,
        WINDOW_WIDTH * sizeof(uint32_t)
    );
    SDL_RenderCopy(sdl_renderer, framebuffer_texture, NULL, NULL);
    SDL_RenderPresent(sdl_renderer);
}

void display_destroy(void) {
    if (framebuffer_texture) SDL_DestroyTexture(framebuffer_texture);
    if (sdl_renderer)        SDL_DestroyRenderer(sdl_renderer);
    if (window)              SDL_DestroyWindow(window);
}
