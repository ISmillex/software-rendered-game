#include "flags.h"
#include "display.h"
#include <string.h>

GameFlags g_flags = {
    .fog_enabled        = true,
    .fly_mode           = false,
    .show_wireframe     = false,
    .show_zbuffer       = false,
    .show_chunk_borders = false,
};

static void cmd_fog(int argc, const char **argv) {
    if (argc < 2) {
        g_flags.fog_enabled = !g_flags.fog_enabled;
    } else if (strcmp(argv[1], "on") == 0) {
        g_flags.fog_enabled = true;
    } else if (strcmp(argv[1], "off") == 0) {
        g_flags.fog_enabled = false;
    }
    if (g_console) {
        console_printf(g_console, COLOR_RGB(255, 255, 0), "fog: %s",
                      g_flags.fog_enabled ? "ON" : "OFF");
    }
}

static void cmd_fly(int argc, const char **argv) {
    (void)argc; (void)argv;
    g_flags.fly_mode = !g_flags.fly_mode;
    if (g_console) {
        console_printf(g_console, COLOR_RGB(255, 255, 0), "fly: %s",
                      g_flags.fly_mode ? "ON" : "OFF");
    }
}

static void cmd_wireframe(int argc, const char **argv) {
    (void)argc; (void)argv;
    g_flags.show_wireframe = !g_flags.show_wireframe;
    if (g_console) {
        console_printf(g_console, COLOR_RGB(255, 255, 0), "wireframe: %s",
                      g_flags.show_wireframe ? "ON" : "OFF");
    }
}

static void cmd_zbuffer(int argc, const char **argv) {
    (void)argc; (void)argv;
    g_flags.show_zbuffer = !g_flags.show_zbuffer;
    if (g_console) {
        console_printf(g_console, COLOR_RGB(255, 255, 0), "zbuffer: %s",
                      g_flags.show_zbuffer ? "ON" : "OFF");
    }
}

void flags_register_commands(Console *con) {
    console_register_command(con, "fog",       "Toggle fog rendering",    cmd_fog);
    console_register_command(con, "fly",       "Toggle fly mode",         cmd_fly);
    console_register_command(con, "wireframe", "Toggle wireframe mode",   cmd_wireframe);
    console_register_command(con, "zbuffer",   "Show depth buffer",       cmd_zbuffer);
}
