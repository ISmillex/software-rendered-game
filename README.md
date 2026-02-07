# Software Rendered Game

A 3D game engine written in C that renders everything on the CPU. There are no GPU graphics APIs involved — no OpenGL, no Vulkan, no Metal. Every pixel that appears on screen is computed by C code and written into a flat array of integers. SDL2 is used only to create a window and copy that array to the screen once per frame. It does nothing else.

## How It Works

The renderer operates on a framebuffer: a one-dimensional array of pixels laid out in row-major order, paired with a depth buffer of the same size. These two arrays are the only destination for all rendering output. Everything visible on screen was written to them by the engine's own code.

### From Objects to Pixels

The scene is made up of objects, each referencing a model (loaded from OBJ files at startup) and a texture (loaded from BMP files). Each frame, the engine walks through every object, transforms its triangles from their local coordinate space into screen coordinates using standard matrix math (model transform, then the camera's combined view-projection matrix), and produces a list of chunks.

A chunk is a single triangle ready to be drawn, along with the information needed to draw it — its screen-space vertices, a depth value for sorting, and either a solid color or a texture with UV coordinates. Chunks are the fundamental unit of rendering work in this engine. They serve the same role as draw calls in a GPU pipeline. Different chunk types map to different rasterizer functions, so adding a new rendering style means adding a new chunk type and writing its rasterizer.

After all chunks are generated, they are sorted front-to-back by depth. This ordering matters because the rasterizers check the depth buffer before writing each pixel. If a closer surface has already been drawn at a given pixel, the rasterizer skips the current one. Sorting front-to-back makes this early rejection happen as often as possible, which saves work.

### Parallel Rendering

The screen is divided into vertical strips, one per CPU core. Each strip covers a contiguous range of columns. After sorting, each chunk is assigned to whichever strips its bounding box overlaps. A triangle that spans two strips goes into both buckets.

A pool of worker threads, created once at startup, renders these strips in parallel. Each thread draws only the pixels within its own column range. Because the strips do not overlap, no thread ever writes to another thread's region of the framebuffer. This eliminates the need for locks or atomic operations on the pixel data. Synchronization is handled by a single pthread barrier: the main thread fills the strip buckets, releases the barrier, the workers render, and the barrier fires again when all workers are done.

Vertical strips were chosen over horizontal ones because of memory layout. The framebuffer is stored row by row, so pixels in the same row are adjacent in memory. A vertical strip accesses sequential addresses within each row, which is cache-friendly. Horizontal strips would jump across rows, scattering memory access.

### Memory

There is no dynamic memory allocation during rendering. A 4 MB arena is allocated once at startup. Each frame, the arena's offset is reset to zero, and all per-frame data — the chunk array, temporary vertex buffers — is bump-allocated from it. This is a simple scheme: allocating means advancing a pointer, and freeing means resetting that pointer to the start. There are no individual frees, no fragmentation, and no calls to malloc or free in the hot path.

Long-lived data — models, textures, the framebuffer itself, the depth buffer, the glyph cache, strip bucket arrays — is allocated once at startup and never freed until shutdown.

### Text and Overlays

Text rendering uses a glyph cache. At startup, every printable ASCII character is rasterized from a bitmap font into an atlas — a single image containing all glyphs at known positions. Drawing a character means copying a rectangle from the atlas to the framebuffer. There is no per-frame font processing.

The engine has several debug overlays drawn on top of the 3D scene after rendering but before presenting to SDL. F1 shows the current state of all game flags. F3 shows frame rate, camera position, and chunk count. F4 shows how many chunks each strip processed. The tilde key opens a console where commands can be typed.

### Console and Flags

The debug console works through a command registry. Each command is a name paired with a callback function. Game flags — fog, fly mode, wireframe rendering, depth buffer visualization — are registered as console commands at startup. Typing "fog" or "fly" in the console invokes the corresponding callback. The console maintains a ring buffer of messages and handles its own text input, cursor movement, and scrolling.

When the console is open, keyboard input is routed to it instead of to the camera and game controls.

### Input

SDL events are polled once per frame into an InputState struct that tracks which keys are currently held and which were pressed this frame (edge-triggered). The rest of the engine reads from this struct and never touches SDL's event system directly.

## Building and Running

The build system is [nob](https://github.com/tsoding/nob.h), a header-only library for writing build recipes in C by Tsoding. There is no Makefile or CMake.

```
cc -o nob nob.c
./nob
./build/game
```

For a debug build with AddressSanitizer and UndefinedBehaviorSanitizer:

```
./nob debug
```

To generate procedural assets:

```
./nob assets
```

The only external dependency is SDL2. On macOS, install it through Homebrew. The engine also links against pthreads and the standard math library.

## Controls

- WASD to move, mouse to look
- F1 shows game flags, F3 shows debug info, F4 shows strip statistics
- Tilde (~) opens the debug console
- Escape to quit

## Acknowledgements

The original idea for this project comes from the [Software Rendering](https://youtube.com/playlist?list=PLpM-Dvs8t0VaOBDp6cVRLBScgSJ2L8blq&si=Bc3OPmiyg6TPAIEn) series by Tsoding Daily.

## License

This project is licensed under the MIT License.
