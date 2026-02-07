#ifndef HUD_H
#define HUD_H

#include "text.h"
#include "input.h"
#include "camera.h"
#include "flags.h"
#include "strip.h"

typedef struct {
    bool active;
    bool dirty;
} OverlayState;

typedef struct {
    OverlayState flags_overlay;
    OverlayState debug_overlay;
    OverlayState strip_overlay;
} HUD;

void hud_handle_input(HUD *hud, const InputState *input);
void hud_render_flags(const GlyphCache *cache, const GameFlags *flags);
void hud_render_debug(const GlyphCache *cache, float fps,
                       const Camera *cam, int chunk_count);
void hud_render_strips(const GlyphCache *cache, const StripPool *pool);
void hud_render_zbuffer_viz(void);

#endif // HUD_H
