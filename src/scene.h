#ifndef SCENE_H
#define SCENE_H

#include "math_utils.h"
#include "chunk.h"
#include "arena.h"

#define MAX_MODELS 32

typedef struct {
    Vec3 *vertices;
    Vec2 *uvs;
    int  *face_verts;
    int  *face_uvs;
    int   vertex_count;
    int   uv_count;
    int   face_count;
    Texture *texture;
} Model;

typedef struct {
    Vec3 min;
    Vec3 max;
} AABB;

typedef struct {
    Model *model;
    Vec3   position;
    Vec3   rotation;
    Vec3   scale;
    float  anim_time;
    bool   anim_bounce;
    float  anim_speed;
    float  anim_amplitude;
    float  anim_base_y;
    bool   solid;
    AABB   bounds;
} SceneObject;

#define MAX_SCENE_OBJECTS 256

typedef struct Scene {
    SceneObject objects[MAX_SCENE_OBJECTS];
    int         object_count;
    Model       models[MAX_MODELS];
    int         model_count;
} Scene;

void    scene_init(Scene *scene);
Model  *scene_load_model(Scene *scene, const char *obj_path, const char *texture_path);
int     scene_add_object(Scene *scene, Model *model, Vec3 position, Vec3 rotation, Vec3 scale);
void    scene_object_set_solid(Scene *scene, int idx);
AABB    scene_object_compute_aabb(const SceneObject *obj);
void    scene_update(Scene *scene, float dt);
void    scene_generate_chunks(const Scene *scene, const Mat4 *vp, Arena *arena,
                              Chunk *chunks, int *chunk_count, int max_chunks);
void    scene_destroy(Scene *scene);

Texture *texture_load_bmp(const char *path);
Mat4     scene_object_model_matrix(const SceneObject *obj);

#endif // SCENE_H
