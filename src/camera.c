#include "camera.h"

void camera_init(Camera *cam) {
    cam->position   = vec3(0.0f, 1.0f, 5.0f);
    cam->yaw        = 0.0f;
    cam->pitch      = 0.0f;
    cam->fov        = deg_to_rad(60.0f);
    cam->near_plane = 0.1f;
    cam->far_plane  = 100.0f;
    cam->move_speed = 5.0f;
    cam->mouse_sens = 0.003f;
    cam->fly_mode   = false;
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

    if (input->keys_down[SDL_SCANCODE_SPACE])  movement.y += 1.0f;
    if (input->keys_down[SDL_SCANCODE_LSHIFT]) movement.y -= 1.0f;

    if (vec3_length(movement) > 0.0f) {
        movement = vec3_normalize(movement);
        cam->position = vec3_add(cam->position,
                                 vec3_scale(movement, cam->move_speed * dt));
    }
}

Mat4 camera_view_matrix(const Camera *cam) {
    Vec3 direction = vec3(
        sinf(cam->yaw) * cosf(cam->pitch),
        sinf(cam->pitch),
        -cosf(cam->yaw) * cosf(cam->pitch)
    );
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
