#ifndef FLAGS_H
#define FLAGS_H

#include "console.h"
#include "camera.h"
#include <stdbool.h>

typedef struct {
    bool fog_enabled;
    bool fly_mode;
    bool noclip;
    bool show_wireframe;
    bool show_zbuffer;
    bool show_chunk_borders;
    bool third_person;
    bool gravity_enabled;
} GameFlags;

extern GameFlags g_flags;
extern Camera *g_camera;

void flags_register_commands(Console *con);

#endif // FLAGS_H
