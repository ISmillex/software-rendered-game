#include "player.h"
#include "display.h"
#include <string.h>
#include <math.h>

#define PLAYER_EYE_HEIGHT 1.0f

Player *g_player = NULL;

static const struct {
    const char *name;
    const char *obj_path;
    const char *tex_path;
    float       scale;
} player_model_defs[MAX_PLAYER_MODELS] = {
    { "penger",      "assets/models/penger-obj/penger/penger-no-hull.obj",
                     "assets/models/penger-obj/penger/penger.png",           0.82f },
    { "cyber",       "assets/models/penger-obj/cyber/cyber-penger.obj",
                     "assets/models/penger-obj/cyber/cyber-penger.png",      0.77f },
    { "real-penger", "assets/models/penger-obj/real-penger/real-penger.obj",
                     "assets/models/penger-obj/real-penger/real-penger.png",  0.30f },
    { "suitger",     "assets/models/penger-obj/suitger/suitedpenger.obj",
                     "assets/models/penger-obj/suitger/suitedpenger.png",    0.90f },
};

void player_init(Player *p, Scene *scene) {
    memset(p, 0, sizeof(Player));
    p->scene          = scene;
    p->scene_obj_idx  = -1;
    p->active_model   = 0;
    p->visible        = false;

    // Load all model variants
    for (int i = 0; i < MAX_PLAYER_MODELS; i++) {
        p->models[i] = scene_load_model(scene,
            player_model_defs[i].obj_path,
            player_model_defs[i].tex_path);
        strncpy(p->model_names[i], player_model_defs[i].name, 31);
        p->model_names[i][31] = '\0';
        p->model_scales[i] = player_model_defs[i].scale;
        if (p->models[i]) {
            p->model_count++;
        } else {
            fprintf(stderr, "Warning: Could not load player model '%s'\n",
                    player_model_defs[i].name);
        }
    }

    // Create scene object with default model
    if (p->models[0]) {
        float s = p->model_scales[0];
        p->scene_obj_idx = scene_add_object(scene, p->models[0],
            vec3(0, 0, 0), vec3(0, 0, 0), vec3(s, s, s));
        if (p->scene_obj_idx >= 0) {
            scene->objects[p->scene_obj_idx].visible = false;
        }
    }

    g_player = p;
}

void player_update(Player *p, Scene *scene, const Camera *cam) {
    if (p->scene_obj_idx < 0) return;

    SceneObject *obj = &scene->objects[p->scene_obj_idx];
    obj->visible = p->visible;

    // Position at player's feet, facing camera yaw direction
    obj->position = vec3(
        cam->position.x,
        cam->position.y - PLAYER_EYE_HEIGHT,
        cam->position.z
    );
    obj->rotation = vec3(0, -cam->yaw + (float)M_PI, 0);
}

void player_set_model(Player *p, int model_index) {
    if (model_index < 0 || model_index >= MAX_PLAYER_MODELS) return;
    if (!p->models[model_index]) return;
    if (p->scene_obj_idx < 0 || !p->scene) return;

    p->active_model = model_index;
    p->scene->objects[p->scene_obj_idx].model = p->models[model_index];
    float s = p->model_scales[model_index];
    p->scene->objects[p->scene_obj_idx].scale = vec3(s, s, s);
}

static void cmd_model(int argc, const char **argv) {
    if (!g_player || !g_console) return;

    if (argc < 2) {
        console_printf(g_console, COLOR_RGB(255, 255, 0), "Current model: %s",
                      g_player->model_names[g_player->active_model]);
        console_printf(g_console, COLOR_RGB(200, 200, 200),
                      "Usage: model <penger|cyber|real-penger|suitger>");
        return;
    }

    for (int i = 0; i < MAX_PLAYER_MODELS; i++) {
        if (strcmp(argv[1], g_player->model_names[i]) == 0) {
            if (!g_player->models[i]) {
                console_printf(g_console, COLOR_RGB(255, 100, 100),
                              "Model '%s' failed to load", argv[1]);
                return;
            }
            player_set_model(g_player, i);
            console_printf(g_console, COLOR_RGB(255, 255, 0), "model: %s", argv[1]);
            return;
        }
    }

    console_printf(g_console, COLOR_RGB(255, 100, 100), "Unknown model: %s", argv[1]);
    console_printf(g_console, COLOR_RGB(200, 200, 200),
                  "Available: penger, cyber, real-penger, suitger");
}

void player_register_commands(Console *con) {
    console_register_command(con, "model", "Change player model", cmd_model);
}
