#include "camera.h"
#include "scene.h"
#include "flags.h"

#define PLAYER_EYE_HEIGHT 1.0f
#define PLAYER_RADIUS     0.2f

void camera_init(Camera *cam) {
    cam->position   = vec3(0.0f, 1.0f, 5.0f);
    cam->yaw        = 0.0f;
    cam->pitch      = 0.0f;
    cam->fov        = deg_to_rad(60.0f);
    cam->near_plane = 0.1f;
    cam->far_plane  = 100.0f;
    cam->move_speed = 5.0f;
    cam->mouse_sens = 0.003f;
    cam->fly_mode     = false;
    cam->third_person = false;
    cam->tp_distance  = 4.0f;
    cam->tp_height    = 1.5f;
}

void camera_handle_input(Camera *cam, const InputState *input, float dt) {
    cam->yaw   += input->mouse_dx * cam->mouse_sens;
    cam->pitch -= input->mouse_dy * cam->mouse_sens;
    cam->pitch  = clampf(cam->pitch, deg_to_rad(-89.0f), deg_to_rad(89.0f));

    Vec3 forward = vec3(sinf(cam->yaw), 0.0f, -cosf(cam->yaw));
    Vec3 right   = vec3(cosf(cam->yaw), 0.0f,  sinf(cam->yaw));

    if (cam->fly_mode) {
        forward = vec3(
            sinf(cam->yaw) * cosf(cam->pitch),
            sinf(cam->pitch),
            -cosf(cam->yaw) * cosf(cam->pitch)
        );
    }

    Vec3 movement = vec3(0, 0, 0);

    if (input->keys_down[SDL_SCANCODE_W]) movement = vec3_add(movement, forward);
    if (input->keys_down[SDL_SCANCODE_S]) movement = vec3_sub(movement, forward);
    if (input->keys_down[SDL_SCANCODE_D]) movement = vec3_add(movement, right);
    if (input->keys_down[SDL_SCANCODE_A]) movement = vec3_sub(movement, right);

    if (cam->fly_mode) {
        if (input->keys_down[SDL_SCANCODE_SPACE])  movement.y += 1.0f;
        if (input->keys_down[SDL_SCANCODE_LSHIFT]) movement.y -= 1.0f;
    }

    if (vec3_length(movement) > 0.0f) {
        movement = vec3_normalize(movement);
        cam->position = vec3_add(cam->position,
                                 vec3_scale(movement, cam->move_speed * dt));
    }
}

void camera_apply_collision(Camera *cam, const Scene *scene) {
    if (cam->fly_mode || g_flags.noclip) return;

    // Ground constraint: keep feet at or above y=0
    if (cam->position.y < PLAYER_EYE_HEIGHT) {
        cam->position.y = PLAYER_EYE_HEIGHT;
    }

    // Collide against solid scene objects
    for (int i = 0; i < scene->object_count; i++) {
        const SceneObject *obj = &scene->objects[i];
        if (!obj->solid) continue;

        AABB bb = obj->bounds;

        // Check Y overlap (player feet to head vs AABB)
        float feet_y = cam->position.y - PLAYER_EYE_HEIGHT;
        float head_y = cam->position.y + 0.1f;
        if (feet_y >= bb.max.y || head_y <= bb.min.y) continue;

        // XZ circle-vs-AABB test
        float closest_x = clampf(cam->position.x, bb.min.x, bb.max.x);
        float closest_z = clampf(cam->position.z, bb.min.z, bb.max.z);

        float dx = cam->position.x - closest_x;
        float dz = cam->position.z - closest_z;
        float dist_sq = dx * dx + dz * dz;

        if (dist_sq < PLAYER_RADIUS * PLAYER_RADIUS) {
            float dist = sqrtf(dist_sq);
            if (dist > 1e-6f) {
                float penetration = PLAYER_RADIUS - dist;
                cam->position.x += (dx / dist) * penetration;
                cam->position.z += (dz / dist) * penetration;
            } else {
                // Player center is inside the AABB — push out along shortest axis
                float push_xn = cam->position.x - bb.min.x;
                float push_xp = bb.max.x - cam->position.x;
                float push_zn = cam->position.z - bb.min.z;
                float push_zp = bb.max.z - cam->position.z;
                float min_push = minf(minf(push_xn, push_xp), minf(push_zn, push_zp));
                if (min_push == push_xn)      cam->position.x = bb.min.x - PLAYER_RADIUS;
                else if (min_push == push_xp) cam->position.x = bb.max.x + PLAYER_RADIUS;
                else if (min_push == push_zn) cam->position.z = bb.min.z - PLAYER_RADIUS;
                else                          cam->position.z = bb.max.z + PLAYER_RADIUS;
            }
        }
    }
}

Mat4 camera_view_matrix(const Camera *cam) {
    Vec3 direction = vec3(
        sinf(cam->yaw) * cosf(cam->pitch),
        sinf(cam->pitch),
        -cosf(cam->yaw) * cosf(cam->pitch)
    );

    if (cam->third_person) {
        // Camera orbits behind and above the player
        Vec3 eye = vec3(
            cam->position.x - direction.x * cam->tp_distance,
            cam->position.y + cam->tp_height,
            cam->position.z - direction.z * cam->tp_distance
        );
        return mat4_look_at(eye, cam->position, vec3(0, 1, 0));
    }

    Vec3 target = vec3_add(cam->position, direction);
    return mat4_look_at(cam->position, target, vec3(0, 1, 0));
}

Mat4 camera_projection_matrix(const Camera *cam, float aspect) {
    return mat4_perspective(cam->fov, aspect, cam->near_plane, cam->far_plane);
}

Mat4 camera_vp_matrix(const Camera *cam, float aspect) {
    Mat4 view = camera_view_matrix(cam);
    Mat4 proj = camera_projection_matrix(cam, aspect);
    return mat4_multiply(proj, view);
}
