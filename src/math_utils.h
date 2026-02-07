#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <math.h>
#include <float.h>

typedef struct { float x, y, z; } Vec3;
typedef struct { float x, y, z, w; } Vec4;
typedef struct { float x, y; } Vec2;
typedef struct { float m[4][4]; } Mat4;  // row-major: m[row][col]

// --- Vec2 Operations ---
static inline Vec2 vec2(float x, float y) {
    return (Vec2){x, y};
}

static inline Vec2 vec2_lerp(Vec2 a, Vec2 b, float t) {
    return (Vec2){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

// --- Vec3 Operations ---
static inline Vec3 vec3(float x, float y, float z) {
    return (Vec3){x, y, z};
}

static inline Vec3 vec3_add(Vec3 a, Vec3 b) {
    return (Vec3){ a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return (Vec3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline Vec3 vec3_scale(Vec3 v, float s) {
    return (Vec3){ v.x * s, v.y * s, v.z * s };
}

static inline Vec3 vec3_negate(Vec3 v) {
    return (Vec3){ -v.x, -v.y, -v.z };
}

static inline float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static inline float vec3_length(Vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline Vec3 vec3_normalize(Vec3 v) {
    float len = vec3_length(v);
    if (len < 1e-8f) return (Vec3){0, 0, 0};
    float inv = 1.0f / len;
    return (Vec3){ v.x * inv, v.y * inv, v.z * inv };
}

static inline Vec3 vec3_lerp(Vec3 a, Vec3 b, float t) {
    return (Vec3){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

// --- Vec4 Operations ---
static inline Vec4 vec4(float x, float y, float z, float w) {
    return (Vec4){x, y, z, w};
}

static inline Vec4 vec4_from_vec3(Vec3 v, float w) {
    return (Vec4){ v.x, v.y, v.z, w };
}

static inline Vec3 vec4_to_vec3(Vec4 v) {
    return (Vec3){ v.x, v.y, v.z };
}

static inline Vec3 vec4_perspective_divide(Vec4 v) {
    if (fabsf(v.w) < 1e-8f) return (Vec3){0, 0, 0};
    float inv_w = 1.0f / v.w;
    return (Vec3){ v.x * inv_w, v.y * inv_w, v.z * inv_w };
}

static inline Vec4 vec4_lerp(Vec4 a, Vec4 b, float t) {
    return (Vec4){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    };
}

// --- Mat4 Operations ---
static inline Mat4 mat4_identity(void) {
    Mat4 m = {{{0}}};
    m.m[0][0] = 1.0f;
    m.m[1][1] = 1.0f;
    m.m[2][2] = 1.0f;
    m.m[3][3] = 1.0f;
    return m;
}

static inline Mat4 mat4_multiply(Mat4 a, Mat4 b) {
    Mat4 r = {{{0}}};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            r.m[i][j] = a.m[i][0] * b.m[0][j]
                       + a.m[i][1] * b.m[1][j]
                       + a.m[i][2] * b.m[2][j]
                       + a.m[i][3] * b.m[3][j];
        }
    }
    return r;
}

static inline Vec4 mat4_mul_vec4(Mat4 m, Vec4 v) {
    return (Vec4){
        m.m[0][0]*v.x + m.m[0][1]*v.y + m.m[0][2]*v.z + m.m[0][3]*v.w,
        m.m[1][0]*v.x + m.m[1][1]*v.y + m.m[1][2]*v.z + m.m[1][3]*v.w,
        m.m[2][0]*v.x + m.m[2][1]*v.y + m.m[2][2]*v.z + m.m[2][3]*v.w,
        m.m[3][0]*v.x + m.m[3][1]*v.y + m.m[3][2]*v.z + m.m[3][3]*v.w
    };
}

static inline Mat4 mat4_translate(Vec3 t) {
    Mat4 m = mat4_identity();
    m.m[0][3] = t.x;
    m.m[1][3] = t.y;
    m.m[2][3] = t.z;
    return m;
}

static inline Mat4 mat4_scale_uniform(float s) {
    Mat4 m = mat4_identity();
    m.m[0][0] = s;
    m.m[1][1] = s;
    m.m[2][2] = s;
    return m;
}

static inline Mat4 mat4_rotate_x(float radians) {
    Mat4 m = mat4_identity();
    float c = cosf(radians);
    float s = sinf(radians);
    m.m[1][1] = c;
    m.m[1][2] = -s;
    m.m[2][1] = s;
    m.m[2][2] = c;
    return m;
}

static inline Mat4 mat4_rotate_y(float radians) {
    Mat4 m = mat4_identity();
    float c = cosf(radians);
    float s = sinf(radians);
    m.m[0][0] = c;
    m.m[0][2] = s;
    m.m[2][0] = -s;
    m.m[2][2] = c;
    return m;
}

static inline Mat4 mat4_rotate_z(float radians) {
    Mat4 m = mat4_identity();
    float c = cosf(radians);
    float s = sinf(radians);
    m.m[0][0] = c;
    m.m[0][1] = -s;
    m.m[1][0] = s;
    m.m[1][1] = c;
    return m;
}

static inline Mat4 mat4_perspective(float fov_radians, float aspect, float near, float far) {
    Mat4 m = {{{0}}};
    float f = 1.0f / tanf(fov_radians / 2.0f);
    m.m[0][0] = f / aspect;
    m.m[1][1] = f;
    m.m[2][2] = (far + near) / (near - far);
    m.m[2][3] = (2.0f * far * near) / (near - far);
    m.m[3][2] = -1.0f;
    return m;
}

static inline Mat4 mat4_look_at(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 forward = vec3_normalize(vec3_sub(target, eye));
    Vec3 right   = vec3_normalize(vec3_cross(forward, up));
    Vec3 true_up = vec3_cross(right, forward);

    Mat4 m = mat4_identity();
    m.m[0][0] = right.x;
    m.m[0][1] = right.y;
    m.m[0][2] = right.z;
    m.m[0][3] = -vec3_dot(right, eye);

    m.m[1][0] = true_up.x;
    m.m[1][1] = true_up.y;
    m.m[1][2] = true_up.z;
    m.m[1][3] = -vec3_dot(true_up, eye);

    m.m[2][0] = -forward.x;
    m.m[2][1] = -forward.y;
    m.m[2][2] = -forward.z;
    m.m[2][3] = vec3_dot(forward, eye);

    return m;
}

// --- Utility Functions ---
static inline float clampf(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

static inline int clampi(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

static inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static inline float deg_to_rad(float degrees) {
    return degrees * (float)M_PI / 180.0f;
}

static inline float rad_to_deg(float radians) {
    return radians * 180.0f / (float)M_PI;
}

static inline int mini(int a, int b) { return a < b ? a : b; }
static inline int maxi(int a, int b) { return a > b ? a : b; }
static inline float minf(float a, float b) { return a < b ? a : b; }
static inline float maxf(float a, float b) { return a > b ? a : b; }

#endif // MATH_UTILS_H
