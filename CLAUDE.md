# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A 3D game with a fully software-rendered pipeline in C (C11). No GPU graphics APIs — every pixel is computed on the CPU and written to a flat framebuffer. SDL2 is used only as a dumb display pipe (window creation + texture blit).

## Build Commands

```bash
# Bootstrap the build system (first time only)
cc -o nob nob.c

# Build (default, -O3)
./nob

# Build (debug, with AddressSanitizer + UndefinedBehaviorSanitizer)
./nob debug

# Generate procedural assets (crate texture)
./nob assets

# Run the game
./build/game
```

The build system is `nob` — a self-rebuilding C build script. If `nob.c` changes, `./nob` recompiles itself before building the game. No Make, CMake, or shell scripts. There are no tests or linters.

**Dependencies:** SDL2 (via homebrew on macOS), pthreads, libm. No third-party C libraries.

## Architecture

Data flows strictly downward through four layers — no layer reaches upward:

```
Application Layer    →  Game loop, Scene, Camera, Input
Debug/UI Layer       →  Console (Quake-style ~), Game Flags, HUD Overlays (F1/F3/F4)
Rendering Layer      →  Chunks, Strip Distribution, Rasterizers, Glyph Cache
Platform Layer       →  SDL2, pthreads, Arena Allocator, File I/O
```

### Rendering Pipeline (per frame)

1. **Chunk Generation** — Scene objects' triangles are transformed through model + VP matrices and emitted as `Chunk` structs into the frame arena. Each chunk = one triangle + its rasterizer type (`CHUNK_COLORED` or `CHUNK_TEXTURED`).
2. **Sort** — Chunks sorted front-to-back by depth key (enables early Z rejection).
3. **Strip Distribution** — Screen is divided into N vertical strips (N = CPU cores). Each chunk is bucketed into the strip(s) its AABB overlaps.
4. **Parallel Render** — N worker threads rasterize their strip buckets. Each thread exclusively owns its x-range of the framebuffer — no locks needed (spatial partitioning).
5. **Overlay Compositing** — HUD/console drawn directly to framebuffer after 3D render.
6. **Present** — `display[]` uploaded to SDL texture.

### Key Design Patterns

- **Chunk = software draw call.** Adding a new visual effect = new chunk type + rasterizer function. This is the "software shader" abstraction.
- **Arena allocator** (`arena.c`): 4MB pre-allocated pool. All per-frame data (chunks, vertex buffers) bump-allocated here. Reset every frame. Zero malloc/free in the hot path.
- **Glyph cache** (`text.c`): All ASCII glyphs pre-rasterized to a bitmap atlas at startup. Text rendering = blitting rectangles. No per-frame font work.
- **Command registry** (`console.c`): Debug console maps command strings to `void (*)(int argc, char **argv)` callbacks. Game flags are exposed as commands (e.g. `fog on`, `fly toggle`).
- **Thread pool** (`strip.c`): Created once at startup, synchronized via pthread barriers. No per-frame thread creation.

### Key Constants

- Window: 800x600, ARGB8888
- Frame arena: 4MB
- Max chunks/frame: 16,384
- Max scene objects: 256, max models: 32
- Thread count: auto-detected CPU cores (clamped 1–16)

## Source Layout

All source is in `src/`. The codebase is ~1,900 lines of game code + ~6,200 lines of math library.

- `main.c` — Entry point, initialization sequence, game loop
- `display.c/h` — Framebuffer (`uint32_t display[]`) and z-buffer, SDL texture upload
- `camera.c/h` — Camera state, view/projection matrix computation
- `input.c/h` — SDL event polling into `InputState` struct (edge-triggered keys)
- `scene.c/h` — Scene objects, OBJ parser, BMP texture loader, chunk generation
- `chunk.c/h` — Chunk types and front-to-back sorting
- `strip.c/h` — Vertical strip distribution, thread pool, barrier sync
- `raster.c/h` — Triangle rasterizers (colored, textured, wireframe)
- `text.c/h` — Glyph cache initialization, text drawing
- `console.c/h` — Debug console, command registry, input routing
- `flags.c/h` — Game flags struct (`g_flags` global), console command registration
- `hud.c/h` — HUD overlays (flags display, debug info, strip stats, z-buffer viz)
- `arena.c/h` — Frame arena allocator
- `math_utils.h` — Vec2/3/4, Mat4, transforms, projection (header-only, inline)


