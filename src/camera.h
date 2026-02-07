#ifndef CAMERA_H
#define CAMERA_H

#include "math_utils.h"
#include "input.h"

typedef struct Scene Scene;

typedef struct {
    Vec3  position;
    float yaw;
    float pitch;
    float fov;
    float near_plane;
    float far_plane;
    float move_speed;
    float mouse_sens;
    bool  fly_mode;
    bool  third_person;
    float tp_distance;
    float tp_height;
    float velocity_y;
    bool  on_ground;
} Camera;

void camera_init(Camera *cam);
void camera_handle_input(Camera *cam, const InputState *input, float dt);
void camera_apply_collision(Camera *cam, const Scene *scene);
Mat4 camera_view_matrix(const Camera *cam);
Mat4 camera_projection_matrix(const Camera *cam, float aspect);
Mat4 camera_vp_matrix(const Camera *cam, float aspect);

#endif // CAMERA_H
