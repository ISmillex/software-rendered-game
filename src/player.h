#ifndef PLAYER_H
#define PLAYER_H

#include "scene.h"
#include "camera.h"
#include "console.h"

#define MAX_PLAYER_MODELS 4

typedef struct {
    Scene   *scene;
    int      scene_obj_idx;
    Model   *models[MAX_PLAYER_MODELS];
    char     model_names[MAX_PLAYER_MODELS][32];
    float    model_scales[MAX_PLAYER_MODELS];
    int      model_count;
    int      active_model;
    bool     visible;
} Player;

extern Player *g_player;

void player_init(Player *p, Scene *scene);
void player_update(Player *p, Scene *scene, const Camera *cam);
void player_set_model(Player *p, int model_index);
void player_register_commands(Console *con);

#endif // PLAYER_H
