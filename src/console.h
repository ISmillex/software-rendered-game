#ifndef CONSOLE_H
#define CONSOLE_H

#include "text.h"
#include "input.h"
#include <stdint.h>
#include <stdbool.h>

#define CONSOLE_MAX_MESSAGES   256
#define CONSOLE_MAX_MSG_LEN    256
#define CONSOLE_INPUT_MAX_LEN  256
#define CONSOLE_MAX_COMMANDS   64
#define CONSOLE_MAX_ARGS       16

typedef struct {
    char text[CONSOLE_MAX_MSG_LEN];
    uint32_t color;
} ConsoleMessage;

typedef void (*CommandCallback)(int argc, const char **argv);

typedef struct {
    char            name[64];
    char            help[128];
    CommandCallback callback;
} Command;

typedef struct {
    bool            open;

    ConsoleMessage  messages[CONSOLE_MAX_MESSAGES];
    int             msg_head;
    int             msg_count;
    int             scroll_offset;

    char            input_buf[CONSOLE_INPUT_MAX_LEN];
    int             input_len;
    int             input_cursor;

    Command         commands[CONSOLE_MAX_COMMANDS];
    int             command_count;
} Console;

extern Console *g_console;

void console_init(Console *con);
void console_register_command(Console *con, const char *name, const char *help,
                              CommandCallback callback);
void console_print(Console *con, const char *msg, uint32_t color);
void console_printf(Console *con, uint32_t color, const char *fmt, ...);
void console_handle_input(Console *con, const InputState *input);
void console_execute(Console *con, const char *command_line);
void console_render(const Console *con, const GlyphCache *cache);

#endif // CONSOLE_H
