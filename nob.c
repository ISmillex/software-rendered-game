#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER "src/"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    Nob_Cmd cmd = {0};

    if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;

    bool debug = false;
    bool gen_assets = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "debug") == 0) debug = true;
        if (strcmp(argv[i], "assets") == 0) gen_assets = true;
    }

    // Generate assets if requested
    if (gen_assets) {
        nob_cmd_append(&cmd, "cc", "-o", BUILD_FOLDER"gen_assets", "tools/gen_assets.c");
        if (!nob_cmd_run(&cmd)) return 1;
        nob_cmd_append(&cmd, BUILD_FOLDER"gen_assets");
        if (!nob_cmd_run(&cmd)) return 1;
        nob_log(NOB_INFO, "Assets generated.");
    }

    // Compile game
    nob_cmd_append(&cmd, "cc");

    if (debug) {
        nob_cmd_append(&cmd, "-g", "-O0", "-DDEBUG");
        nob_cmd_append(&cmd, "-fsanitize=address,undefined");
    } else {
        nob_cmd_append(&cmd, "-O3");
    }

    nob_cmd_append(&cmd, "-std=c11");
    nob_cmd_append(&cmd, "-Wall", "-Wextra", "-Wno-unused-parameter");

    // Source files
    nob_cmd_append(&cmd,
        SRC_FOLDER"main.c",
        SRC_FOLDER"display.c",
        SRC_FOLDER"camera.c",
        SRC_FOLDER"input.c",
        SRC_FOLDER"chunk.c",
        SRC_FOLDER"strip.c",
        SRC_FOLDER"raster.c",
        SRC_FOLDER"scene.c",
        SRC_FOLDER"text.c",
        SRC_FOLDER"console.c",
        SRC_FOLDER"flags.c",
        SRC_FOLDER"hud.c",
        SRC_FOLDER"arena.c",
        SRC_FOLDER"player.c",
        SRC_FOLDER"physics.c"
    );

    // Include path
    nob_cmd_append(&cmd, "-I"SRC_FOLDER);

    // Libraries
    nob_cmd_append(&cmd, "-lpthread", "-lm");

    // SDL2 flags (macOS uses sdl2-config, Linux uses pkg-config)
#ifdef __APPLE__
    // Try sdl2-config first
    nob_cmd_append(&cmd, "-I/opt/homebrew/include", "-I/usr/local/include");
    nob_cmd_append(&cmd, "-L/opt/homebrew/lib", "-L/usr/local/lib");
    nob_cmd_append(&cmd, "-lSDL2");
#else
    nob_cmd_append(&cmd, "-lSDL2");
#endif

    // Output
    nob_cmd_append(&cmd, "-o", BUILD_FOLDER"game");

    if (!nob_cmd_run(&cmd)) return 1;

    nob_log(NOB_INFO, "Build successful! Run: ./"BUILD_FOLDER"game");

    return 0;
}
