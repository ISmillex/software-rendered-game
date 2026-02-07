#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef struct {
    bool keys_down[SDL_NUM_SCANCODES];
    bool keys_pressed[SDL_NUM_SCANCODES];
    bool keys_released[SDL_NUM_SCANCODES];
    int  mouse_dx;
    int  mouse_dy;
    bool quit_requested;
    char text_input[32];
    int  text_input_len;
} InputState;

void input_init(void);
void input_poll(InputState *state);
bool input_is_key_down(const InputState *state, SDL_Scancode key);
bool input_was_key_pressed(const InputState *state, SDL_Scancode key);
bool input_was_key_released(const InputState *state, SDL_Scancode key);

#endif // INPUT_H
