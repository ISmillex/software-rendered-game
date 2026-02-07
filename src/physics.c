#include "physics.h"
#include "flags.h"
#include <math.h>
#include <string.h>

void physics_init(PhysicsWorld *world) {
    memset(world, 0, sizeof(PhysicsWorld));
}

int physics_add_ball(PhysicsWorld *world, int scene_idx, float radius, float restitution) {
    if (world->body_count >= MAX_PHYSICS_BODIES) return -1;
    int idx = world->body_count++;
    PhysicsBody *b = &world->bodies[idx];
    b->scene_idx   = scene_idx;
    b->velocity    = vec3(0, 0, 0);
    b->radius      = radius;
    b->restitution = restitution;
    b->lifetime    = -1.0f;  // permanent
    b->active      = true;
    b->at_rest     = true;
    return idx;
}

void physics_spawn_stone(PhysicsWorld *world, Scene *scene,
                         Model *stone_model, Vec3 origin, Vec3 direction) {
    // Find an inactive slot
    int slot = -1;
    for (int i = 0; i < world->body_count; i++) {
        if (!world->bodies[i].active) {
            slot = i;
            break;
        }
    }
    // Or append
    if (slot < 0) {
        if (world->body_count >= MAX_PHYSICS_BODIES) {
            // Recycle oldest stone (first active with lifetime > 0)
            for (int i = 0; i < world->body_count; i++) {
                if (world->bodies[i].active && world->bodies[i].lifetime > 0) {
                    int si = world->bodies[i].scene_idx;
                    scene->objects[si].visible = false;
                    scene->objects[si].recyclable = true;
                    scene->objects[si].solid = false;
                    world->bodies[i].active = false;
                    slot = i;
                    break;
                }
            }
            if (slot < 0) return;  // no slot available
        } else {
            slot = world->body_count++;
        }
    }

    // Spawn slightly in front of the player
    float diameter = STONE_RADIUS * 2.0f;
    Vec3 spawn_pos = vec3_add(origin, vec3_scale(direction, 0.5f));

    int idx = scene_add_object(scene, stone_model, spawn_pos,
                               vec3(0, 0, 0), vec3(diameter, diameter, diameter));
    if (idx < 0) return;
    scene_object_set_solid(scene, idx);

    PhysicsBody *b = &world->bodies[slot];
    b->scene_idx   = idx;
    b->velocity    = vec3_scale(direction, STONE_THROW_SPEED);
    b->radius      = STONE_RADIUS;
    b->restitution = 0.5f;
    b->lifetime    = STONE_LIFETIME;
    b->active      = true;
    b->at_rest     = false;
}

// Reflect velocity along a normal: v' = v - 2*(v.n)*n, then scale by restitution
static Vec3 reflect_velocity(Vec3 vel, Vec3 normal, float restitution) {
    float vn = vec3_dot(vel, normal);
    if (vn >= 0.0f) return vel;  // moving away already
    Vec3 reflected = vec3_sub(vel, vec3_scale(normal, 2.0f * vn));
    // Apply restitution only to the normal component
    float reflected_vn = vec3_dot(reflected, normal);
    Vec3 tangent = vec3_sub(reflected, vec3_scale(normal, reflected_vn));
    return vec3_add(vec3_scale(normal, reflected_vn * restitution), tangent);
}

static void collide_sphere_aabb(PhysicsBody *body, Vec3 *pos, AABB bb) {
    // Find closest point on AABB to sphere center
    float cx = clampf(pos->x, bb.min.x, bb.max.x);
    float cy = clampf(pos->y, bb.min.y, bb.max.y);
    float cz = clampf(pos->z, bb.min.z, bb.max.z);

    float dx = pos->x - cx;
    float dy = pos->y - cy;
    float dz = pos->z - cz;
    float dist_sq = dx * dx + dy * dy + dz * dz;

    if (dist_sq < body->radius * body->radius && dist_sq > 1e-8f) {
        float dist = sqrtf(dist_sq);
        Vec3 normal = vec3(dx / dist, dy / dist, dz / dist);
        float penetration = body->radius - dist;

        // Push out
        *pos = vec3_add(*pos, vec3_scale(normal, penetration));

        // Reflect velocity
        body->velocity = reflect_velocity(body->velocity, normal, body->restitution);
    }
}

void physics_update(PhysicsWorld *world, Scene *scene, float dt) {
    for (int i = 0; i < world->body_count; i++) {
        PhysicsBody *body = &world->bodies[i];
        if (!body->active || body->at_rest) continue;

        SceneObject *obj = &scene->objects[body->scene_idx];
        Vec3 pos = obj->position;

        // Gravity
        if (g_flags.gravity_enabled) {
            body->velocity.y -= PHYSICS_GRAVITY * dt;
        }

        // Integration (semi-implicit Euler)
        pos = vec3_add(pos, vec3_scale(body->velocity, dt));

        // Floor collision (y = 0 plane)
        if (pos.y - body->radius < 0.0f) {
            pos.y = body->radius;
            body->velocity = reflect_velocity(body->velocity, vec3(0, 1, 0), body->restitution);

            // Ground friction
            body->velocity.x *= (1.0f - 3.0f * dt);
            body->velocity.z *= (1.0f - 3.0f * dt);

            // Come to rest if very slow
            if (fabsf(body->velocity.y) < 0.5f) {
                body->velocity.y = 0.0f;
                float xz_speed = sqrtf(body->velocity.x * body->velocity.x +
                                       body->velocity.z * body->velocity.z);
                if (xz_speed < 0.1f) {
                    body->velocity = vec3(0, 0, 0);
                    body->at_rest = true;
                }
            }
        }

        // Collide against solid scene objects (walls, cubes)
        for (int j = 0; j < scene->object_count; j++) {
            const SceneObject *sobj = &scene->objects[j];
            if (!sobj->solid || j == body->scene_idx) continue;
            // Skip other physics bodies (handled in sphere-sphere)
            bool is_physics_obj = false;
            for (int k = 0; k < world->body_count; k++) {
                if (world->bodies[k].active && world->bodies[k].scene_idx == j) {
                    is_physics_obj = true;
                    break;
                }
            }
            if (is_physics_obj) continue;

            collide_sphere_aabb(body, &pos, sobj->bounds);
        }

        // Sphere-vs-sphere collision (against other physics bodies)
        for (int j = 0; j < world->body_count; j++) {
            if (i == j || !world->bodies[j].active) continue;
            PhysicsBody *other = &world->bodies[j];
            Vec3 other_pos = scene->objects[other->scene_idx].position;

            Vec3 diff = vec3_sub(pos, other_pos);
            float dist = vec3_length(diff);
            float min_dist = body->radius + other->radius;

            if (dist < min_dist && dist > 1e-6f) {
                Vec3 normal = vec3_scale(diff, 1.0f / dist);
                float penetration = min_dist - dist;

                // Push apart (move this body only for stones hitting balls)
                pos = vec3_add(pos, vec3_scale(normal, penetration * 0.5f));

                // Reflect this body's velocity
                body->velocity = reflect_velocity(body->velocity, normal, body->restitution);
                body->at_rest = false;

                // Give the other body a kick
                if (other->at_rest || other->lifetime < 0) {
                    float impulse = vec3_length(body->velocity) * 0.3f;
                    other->velocity = vec3_add(other->velocity,
                                               vec3_scale(vec3_negate(normal), impulse));
                    other->at_rest = false;
                }

                // Push other body apart too
                scene->objects[other->scene_idx].position =
                    vec3_sub(other_pos, vec3_scale(normal, penetration * 0.5f));
            }
        }

        // Lifetime
        if (body->lifetime > 0) {
            body->lifetime -= dt;
        }

        // Sync position back to scene object
        obj->position = pos;
        if (obj->solid) {
            obj->bounds = scene_object_compute_aabb(obj);
        }
    }
}

void physics_cleanup(PhysicsWorld *world, Scene *scene) {
    for (int i = 0; i < world->body_count; i++) {
        PhysicsBody *body = &world->bodies[i];
        if (!body->active) continue;
        // Permanent bodies have lifetime < 0 (never expire)
        // Stones have lifetime that counts down from positive; expired when <= 0
        if (body->lifetime < 0.0f) continue;
        if (body->lifetime <= 0.0f) {
            SceneObject *obj = &scene->objects[body->scene_idx];
            obj->visible = false;
            obj->solid = false;
            obj->recyclable = true;
            body->active = false;
        }
    }
}

void physics_player_interact(PhysicsWorld *world, Scene *scene, Vec3 player_pos, float player_radius) {
    for (int i = 0; i < world->body_count; i++) {
        PhysicsBody *body = &world->bodies[i];
        if (!body->active) continue;

        Vec3 obj_pos = scene->objects[body->scene_idx].position;
        Vec3 diff = vec3_sub(player_pos, obj_pos);
        // Only check XZ + Y overlap (player is a vertical cylinder)
        diff.y = 0.0f;
        float dist_xz = vec3_length(diff);
        float min_dist = player_radius + body->radius;

        // Check Y overlap: player feet to head vs sphere center ± radius
        float feet_y = player_pos.y - 1.0f;  // PLAYER_EYE_HEIGHT
        float head_y = player_pos.y + 0.1f;
        if (obj_pos.y + body->radius < feet_y || obj_pos.y - body->radius > head_y)
            continue;

        if (dist_xz < min_dist && dist_xz > 1e-6f) {
            // Push the physics body away from the player
            Vec3 push_dir = vec3_scale(diff, 1.0f / dist_xz);
            float penetration = min_dist - dist_xz;

            // Move the body out of the player
            scene->objects[body->scene_idx].position = vec3_add(
                obj_pos, vec3_scale(push_dir, penetration));

            // Give it a velocity kick in the push direction
            float kick = 4.0f;
            body->velocity.x += push_dir.x * kick;
            body->velocity.z += push_dir.z * kick;
            // Small upward nudge so it doesn't just slide
            if (body->velocity.y < 1.0f) body->velocity.y += 1.0f;
            body->at_rest = false;
        }
    }
}
