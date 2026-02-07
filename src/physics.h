#ifndef PHYSICS_H
#define PHYSICS_H

#include "math_utils.h"
#include "scene.h"
#include <stdbool.h>

#define MAX_PHYSICS_BODIES 64
#define STONE_RADIUS       0.08f
#define BALL_RADIUS        0.4f
#define STONE_LIFETIME     8.0f
#define STONE_THROW_SPEED  18.0f
#define THROW_COOLDOWN     0.15f
#define PHYSICS_GRAVITY    20.0f

typedef struct {
    int   scene_idx;
    Vec3  velocity;
    float radius;
    float restitution;
    float lifetime;       // seconds remaining, -1 = permanent
    bool  active;
    bool  at_rest;
} PhysicsBody;

typedef struct {
    PhysicsBody bodies[MAX_PHYSICS_BODIES];
    int         body_count;
    float       throw_cooldown;
} PhysicsWorld;

void physics_init(PhysicsWorld *world);
void physics_update(PhysicsWorld *world, Scene *scene, float dt);
void physics_spawn_stone(PhysicsWorld *world, Scene *scene,
                         Model *stone_model, Vec3 origin, Vec3 direction);
int  physics_add_ball(PhysicsWorld *world, int scene_idx, float radius, float restitution);
void physics_cleanup(PhysicsWorld *world, Scene *scene);
void physics_player_interact(PhysicsWorld *world, Scene *scene, Vec3 player_pos, float player_radius);

#endif // PHYSICS_H
