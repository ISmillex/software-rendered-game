#include "console.h"
#include "display.h"
#include "math_utils.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

Console *g_console = NULL;

void console_init(Console *con) {
    memset(con, 0, sizeof(Console));
    g_console = con;
}

void console_register_command(Console *con, const char *name, const char *help,
                              CommandCallback callback) {
    if (con->command_count >= CONSOLE_MAX_COMMANDS) return;
    Command *cmd = &con->commands[con->command_count++];
    strncpy(cmd->name, name, sizeof(cmd->name) - 1);
    strncpy(cmd->help, help, sizeof(cmd->help) - 1);
    cmd->callback = callback;
}

void console_print(Console *con, const char *msg, uint32_t color) {
    ConsoleMessage *m = &con->messages[con->msg_head];
    strncpy(m->text, msg, CONSOLE_MAX_MSG_LEN - 1);
    m->text[CONSOLE_MAX_MSG_LEN - 1] = '\0';
    m->color = color;

    con->msg_head = (con->msg_head + 1) % CONSOLE_MAX_MESSAGES;
    if (con->msg_count < CONSOLE_MAX_MESSAGES) {
        con->msg_count++;
    }
}

void console_printf(Console *con, uint32_t color, const char *fmt, ...) {
    char buf[CONSOLE_MAX_MSG_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    console_print(con, buf, color);
}

void console_handle_input(Console *con, const InputState *input) {
    if (!con->open) return;

    // Submit command
    if (input_was_key_pressed(input, SDL_SCANCODE_RETURN)) {
        if (con->input_len > 0) {
            con->input_buf[con->input_len] = '\0';
            console_printf(con, COLOR_RGB(200, 200, 200), "> %s", con->input_buf);
            console_execute(con, con->input_buf);
            con->input_len = 0;
            con->input_cursor = 0;
            con->input_buf[0] = '\0';
        }
        return;
    }

    // Backspace
    if (input_was_key_pressed(input, SDL_SCANCODE_BACKSPACE)) {
        if (con->input_cursor > 0) {
            memmove(&con->input_buf[con->input_cursor - 1],
                    &con->input_buf[con->input_cursor],
                    con->input_len - con->input_cursor);
            con->input_cursor--;
            con->input_len--;
            con->input_buf[con->input_len] = '\0';
        }
        return;
    }

    // Scroll
    if (input_was_key_pressed(input, SDL_SCANCODE_PAGEUP)) {
        con->scroll_offset = mini(con->scroll_offset + 5, con->msg_count - 1);
        if (con->scroll_offset < 0) con->scroll_offset = 0;
    }
    if (input_was_key_pressed(input, SDL_SCANCODE_PAGEDOWN)) {
        con->scroll_offset = maxi(con->scroll_offset - 5, 0);
    }

    // Text input from SDL_TEXTINPUT events
    for (int i = 0; i < input->text_input_len; i++) {
        char c = input->text_input[i];
        // Filter out backtick (console toggle key) and non-printable chars
        if (c == '`' || c == '~') continue;
        if (c < 32 || c > 126) continue;
        if (con->input_len < CONSOLE_INPUT_MAX_LEN - 1) {
            // Insert at cursor
            memmove(&con->input_buf[con->input_cursor + 1],
                    &con->input_buf[con->input_cursor],
                    con->input_len - con->input_cursor);
            con->input_buf[con->input_cursor] = c;
            con->input_cursor++;
            con->input_len++;
            con->input_buf[con->input_len] = '\0';
        }
    }
}

void console_execute(Console *con, const char *command_line) {
    char buf[CONSOLE_INPUT_MAX_LEN];
    strncpy(buf, command_line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    // Trim leading/trailing whitespace and control characters
    char *start = buf;
    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') start++;
    char *end = start + strlen(start);
    while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) end--;
    *end = '\0';

    const char *argv[CONSOLE_MAX_ARGS];
    int argc = 0;

    char *token = strtok(start, " \t");
    while (token != NULL && argc < CONSOLE_MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }

    if (argc == 0) return;

    // Built-in help command
    if (strcmp(argv[0], "help") == 0) {
        console_printf(con, COLOR_RGB(255, 255, 0), "Available commands:");
        for (int i = 0; i < con->command_count; i++) {
            console_printf(con, COLOR_RGB(200, 200, 200), "  %s - %s",
                          con->commands[i].name, con->commands[i].help);
        }
        console_printf(con, COLOR_RGB(200, 200, 200), "  help - Show this message");
        return;
    }

    for (int i = 0; i < con->command_count; i++) {
        if (strcmp(con->commands[i].name, argv[0]) == 0) {
            con->commands[i].callback(argc, argv);
            return;
        }
    }

    console_printf(con, COLOR_RGB(255, 100, 100), "Unknown command: %s", argv[0]);
}

void console_render(const Console *con, const GlyphCache *cache) {
    if (!con->open) return;

    int console_height = WINDOW_HEIGHT / 2;

    // Semi-transparent background
    for (int y = 0; y < console_height; y++) {
        for (int x = 0; x < WINDOW_WIDTH; x++) {
            int idx = y * WINDOW_WIDTH + x;
            uint32_t pixel = display_buffer[idx];
            uint32_t r = ((pixel >> 16) & 0xFF) >> 1;
            uint32_t g = ((pixel >>  8) & 0xFF) >> 1;
            uint32_t b = ((pixel)       & 0xFF) >> 1;
            display_buffer[idx] = COLOR_RGB(r, g, b);
        }
    }

    // Separator line
    int sep_y = console_height - 1;
    for (int x = 0; x < WINDOW_WIDTH; x++) {
        display_buffer[sep_y * WINDOW_WIDTH + x] = COLOR_RGB(200, 200, 200);
    }

    // Input line
    int input_y = console_height - cache->glyph_height - 4;
    text_draw_string(cache, "> ", 4, input_y, COLOR_RGB(200, 200, 200));
    text_draw_string(cache, con->input_buf, 4 + 2 * cache->glyph_width, input_y,
                     COLOR_RGB(255, 255, 255));

    // Cursor
    int cursor_x = 4 + (2 + con->input_cursor) * cache->glyph_width;
    for (int row = 0; row < cache->glyph_height; row++) {
        int cy = input_y + row;
        if (cy >= 0 && cy < WINDOW_HEIGHT && cursor_x >= 0 && cursor_x < WINDOW_WIDTH) {
            display_buffer[cy * WINDOW_WIDTH + cursor_x] = COLOR_RGB(255, 255, 255);
        }
    }

    // Messages (bottom-up)
    int max_visible_lines = (input_y - 4) / cache->glyph_height;
    int msg_y = input_y - cache->glyph_height - 4;

    for (int i = 0; i < max_visible_lines && i < con->msg_count; i++) {
        int msg_index = (con->msg_head - 1 - i - con->scroll_offset
                         + CONSOLE_MAX_MESSAGES * 2) % CONSOLE_MAX_MESSAGES;
        if (i + con->scroll_offset >= con->msg_count) break;

        text_draw_string(cache, con->messages[msg_index].text,
                         4, msg_y, con->messages[msg_index].color);
        msg_y -= cache->glyph_height;
    }
}
