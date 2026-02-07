#include "input.h"
#include <string.h>

void input_init(void) {
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_StartTextInput();
}

void input_poll(InputState *state) {
    memset(state->keys_pressed, 0, sizeof(state->keys_pressed));
    memset(state->keys_released, 0, sizeof(state->keys_released));
    state->mouse_dx = 0;
    state->mouse_dy = 0;
    state->text_input_len = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                state->quit_requested = true;
                break;

            case SDL_KEYDOWN:
                if (!event.key.repeat) {
                    state->keys_pressed[event.key.keysym.scancode] = true;
                }
                state->keys_down[event.key.keysym.scancode] = true;
                break;

            case SDL_KEYUP:
                state->keys_down[event.key.keysym.scancode] = false;
                state->keys_released[event.key.keysym.scancode] = true;
                break;

            case SDL_MOUSEMOTION:
                state->mouse_dx += event.motion.xrel;
                state->mouse_dy += event.motion.yrel;
                break;

            case SDL_TEXTINPUT:
                for (int i = 0; event.text.text[i] != '\0' && state->text_input_len < 31; i++) {
                    state->text_input[state->text_input_len++] = event.text.text[i];
                }
                break;
        }
    }
    state->text_input[state->text_input_len] = '\0';
}

bool input_is_key_down(const InputState *state, SDL_Scancode key) {
    return state->keys_down[key];
}

bool input_was_key_pressed(const InputState *state, SDL_Scancode key) {
    return state->keys_pressed[key];
}

bool input_was_key_released(const InputState *state, SDL_Scancode key) {
    return state->keys_released[key];
}
