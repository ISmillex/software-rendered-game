#include "scene.h"
#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void scene_init(Scene *scene) {
    memset(scene, 0, sizeof(Scene));
}

// --- BMP Loader ---
Texture *texture_load_bmp(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open texture: %s\n", path);
        return NULL;
    }

    unsigned char header[54];
    if (fread(header, 1, 54, f) != 54) {
        fprintf(stderr, "Invalid BMP header: %s\n", path);
        fclose(f);
        return NULL;
    }

    if (header[0] != 'B' || header[1] != 'M') {
        fprintf(stderr, "Not a BMP file: %s\n", path);
        fclose(f);
        return NULL;
    }

    int data_offset = *(int *)&header[10];
    int width       = *(int *)&header[18];
    int height_raw  = *(int *)&header[22];
    int bpp         = *(short *)&header[28];

    bool top_down = height_raw < 0;
    int height = top_down ? -height_raw : height_raw;

    if (bpp != 24 && bpp != 32) {
        fprintf(stderr, "Unsupported BMP bpp %d: %s\n", bpp, path);
        fclose(f);
        return NULL;
    }

    fseek(f, data_offset, SEEK_SET);

    int bytes_per_pixel = bpp / 8;
    int row_size = ((width * bytes_per_pixel + 3) / 4) * 4;
    unsigned char *row_data = malloc(row_size);

    Texture *tex = malloc(sizeof(Texture));
    tex->width  = width;
    tex->height = height;
    tex->pixels = malloc(width * height * sizeof(uint32_t));

    for (int y = 0; y < height; y++) {
        if (fread(row_data, 1, row_size, f) != (size_t)row_size) break;
        int dest_y = top_down ? y : (height - 1 - y);
        for (int x = 0; x < width; x++) {
            unsigned char b = row_data[x * bytes_per_pixel + 0];
            unsigned char g = row_data[x * bytes_per_pixel + 1];
            unsigned char r = row_data[x * bytes_per_pixel + 2];
            tex->pixels[dest_y * width + x] = COLOR_RGB(r, g, b);
        }
    }

    free(row_data);
    fclose(f);
    return tex;
}

// --- OBJ Loader ---
Model *scene_load_model(Scene *scene, const char *obj_path, const char *texture_path) {
    if (scene->model_count >= MAX_MODELS) {
        fprintf(stderr, "Max models reached\n");
        return NULL;
    }

    FILE *f = fopen(obj_path, "r");
    if (!f) {
        fprintf(stderr, "Failed to open model: %s\n", obj_path);
        return NULL;
    }

    // First pass: count
    int vert_count = 0, uv_count = 0, face_count = 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == 'v' && line[1] == ' ')       vert_count++;
        else if (line[0] == 'v' && line[1] == 't')  uv_count++;
        else if (line[0] == 'f' && line[1] == ' ')  face_count++;
    }

    Model *model = &scene->models[scene->model_count++];
    model->vertex_count = vert_count;
    model->uv_count     = uv_count;
    model->face_count   = face_count;
    model->vertices     = malloc(vert_count * sizeof(Vec3));
    model->uvs          = uv_count > 0 ? malloc(uv_count * sizeof(Vec2)) : NULL;
    model->face_verts   = malloc(face_count * 3 * sizeof(int));
    model->face_uvs     = malloc(face_count * 3 * sizeof(int));

    // Second pass: parse
    rewind(f);
    int vi = 0, ui = 0, fi = 0;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == 'v' && line[1] == ' ') {
            float x, y, z;
            sscanf(line + 2, "%f %f %f", &x, &y, &z);
            model->vertices[vi++] = vec3(x, y, z);
        }
        else if (line[0] == 'v' && line[1] == 't') {
            float u, v;
            sscanf(line + 3, "%f %f", &u, &v);
            model->uvs[ui++] = vec2(u, v);
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            int v0, v1, v2, vt0 = -1, vt1 = -1, vt2 = -1;
            int vn;
            if (sscanf(line + 2, "%d/%d/%d %d/%d/%d %d/%d/%d",
                        &v0, &vt0, &vn, &v1, &vt1, &vn, &v2, &vt2, &vn) >= 9) {
                // v/vt/vn format
            } else if (sscanf(line + 2, "%d/%d %d/%d %d/%d",
                               &v0, &vt0, &v1, &vt1, &v2, &vt2) >= 6) {
                // v/vt format
            } else if (sscanf(line + 2, "%d//%d %d//%d %d//%d",
                               &v0, &vn, &v1, &vn, &v2, &vn) >= 6) {
                // v//vn format (no UVs)
                vt0 = vt1 = vt2 = -1;
            } else {
                sscanf(line + 2, "%d %d %d", &v0, &v1, &v2);
                vt0 = vt1 = vt2 = -1;
            }
            model->face_verts[fi * 3 + 0] = v0 - 1;
            model->face_verts[fi * 3 + 1] = v1 - 1;
            model->face_verts[fi * 3 + 2] = v2 - 1;
            model->face_uvs[fi * 3 + 0] = vt0 > 0 ? vt0 - 1 : -1;
            model->face_uvs[fi * 3 + 1] = vt1 > 0 ? vt1 - 1 : -1;
            model->face_uvs[fi * 3 + 2] = vt2 > 0 ? vt2 - 1 : -1;
            fi++;
        }
    }

    fclose(f);

    // Load texture
    model->texture = NULL;
    if (texture_path) {
        model->texture = texture_load_bmp(texture_path);
    }

    return model;
}

int scene_add_object(Scene *scene, Model *model, Vec3 position, Vec3 rotation, float scale) {
    if (scene->object_count >= MAX_SCENE_OBJECTS) return -1;
    int idx = scene->object_count++;
    SceneObject *obj = &scene->objects[idx];
    obj->model          = model;
    obj->position       = position;
    obj->rotation       = rotation;
    obj->scale          = scale;
    obj->anim_time      = 0.0f;
    obj->anim_bounce    = false;
    obj->anim_speed     = 1.0f;
    obj->anim_amplitude = 0.0f;
    obj->anim_base_y    = position.y;
    return idx;
}

void scene_update(Scene *scene, float dt) {
    for (int i = 0; i < scene->object_count; i++) {
        SceneObject *obj = &scene->objects[i];
        if (obj->anim_bounce) {
            obj->anim_time += dt * obj->anim_speed;
            obj->position.y = obj->anim_base_y + sinf(obj->anim_time) * obj->anim_amplitude;
        }
    }
}

Mat4 scene_object_model_matrix(const SceneObject *obj) {
    Mat4 t  = mat4_translate(obj->position);
    Mat4 rx = mat4_rotate_x(obj->rotation.x);
    Mat4 ry = mat4_rotate_y(obj->rotation.y);
    Mat4 rz = mat4_rotate_z(obj->rotation.z);
    Mat4 s  = mat4_scale_uniform(obj->scale);

    Mat4 result = mat4_multiply(t, mat4_multiply(ry, mat4_multiply(rx, mat4_multiply(rz, s))));
    return result;
}

// Near-plane clipping helper
static void clip_triangle(Vec4 clip[3], Vec2 uvs[3], bool has_uvs, float near,
                           Vec4 out_clip[][3], Vec2 out_uvs[][3], int *out_count) {
    *out_count = 0;

    // Count vertices in front of near plane (w > near threshold)
    // In clip space, a vertex is in front if clip.w > near (since w = -z_view)
    int in_front[3];
    int front_count = 0;
    for (int i = 0; i < 3; i++) {
        in_front[i] = clip[i].w > near ? 1 : 0;
        front_count += in_front[i];
    }

    if (front_count == 0) {
        return; // all behind
    }

    if (front_count == 3) {
        // All in front
        for (int i = 0; i < 3; i++) {
            out_clip[0][i] = clip[i];
            out_uvs[0][i] = uvs[i];
        }
        *out_count = 1;
        return;
    }

    if (front_count == 1) {
        // Find the one in front
        int a = -1;
        for (int i = 0; i < 3; i++) {
            if (in_front[i]) { a = i; break; }
        }
        int b = (a + 1) % 3;
        int c = (a + 2) % 3;

        float t_ab = (near - clip[a].w) / (clip[b].w - clip[a].w);
        float t_ac = (near - clip[a].w) / (clip[c].w - clip[a].w);

        Vec4 clipped_b = vec4_lerp(clip[a], clip[b], t_ab);
        Vec4 clipped_c = vec4_lerp(clip[a], clip[c], t_ac);
        Vec2 uv_b = vec2_lerp(uvs[a], uvs[b], t_ab);
        Vec2 uv_c = vec2_lerp(uvs[a], uvs[c], t_ac);

        out_clip[0][0] = clip[a];
        out_clip[0][1] = clipped_b;
        out_clip[0][2] = clipped_c;
        out_uvs[0][0] = uvs[a];
        out_uvs[0][1] = uv_b;
        out_uvs[0][2] = uv_c;
        *out_count = 1;
        return;
    }

    // front_count == 2
    int behind = -1;
    for (int i = 0; i < 3; i++) {
        if (!in_front[i]) { behind = i; break; }
    }
    int a = (behind + 1) % 3;
    int b = (behind + 2) % 3;

    float t_a = (near - clip[a].w) / (clip[behind].w - clip[a].w);
    float t_b = (near - clip[b].w) / (clip[behind].w - clip[b].w);

    Vec4 clipped_a = vec4_lerp(clip[a], clip[behind], t_a);
    Vec4 clipped_b = vec4_lerp(clip[b], clip[behind], t_b);
    Vec2 uv_ca = vec2_lerp(uvs[a], uvs[behind], t_a);
    Vec2 uv_cb = vec2_lerp(uvs[b], uvs[behind], t_b);

    // Triangle 1: a, b, clipped_a
    out_clip[0][0] = clip[a];
    out_clip[0][1] = clip[b];
    out_clip[0][2] = clipped_a;
    out_uvs[0][0] = uvs[a];
    out_uvs[0][1] = uvs[b];
    out_uvs[0][2] = uv_ca;

    // Triangle 2: b, clipped_b, clipped_a
    out_clip[1][0] = clip[b];
    out_clip[1][1] = clipped_b;
    out_clip[1][2] = clipped_a;
    out_uvs[1][0] = uvs[b];
    out_uvs[1][1] = uv_cb;
    out_uvs[1][2] = uv_ca;

    *out_count = 2;
}

static uint32_t face_color_from_index(int face_idx) {
    // Generate distinct colors for untextured faces
    uint8_t r = (uint8_t)(80 + (face_idx * 37) % 160);
    uint8_t g = (uint8_t)(80 + (face_idx * 73) % 160);
    uint8_t b = (uint8_t)(80 + (face_idx * 113) % 160);
    return COLOR_RGB(r, g, b);
}

void scene_generate_chunks(const Scene *scene, const Mat4 *vp, Arena *arena,
                           Chunk *chunks, int *chunk_count, int max_chunks) {
    (void)arena;
    *chunk_count = 0;

    for (int obj_i = 0; obj_i < scene->object_count; obj_i++) {
        const SceneObject *obj = &scene->objects[obj_i];
        const Model *model = obj->model;
        if (!model) continue;

        Mat4 model_mat = scene_object_model_matrix(obj);
        Mat4 mvp = mat4_multiply(*vp, model_mat);

        for (int f = 0; f < model->face_count; f++) {
            if (*chunk_count >= max_chunks) return;

            int vi0 = model->face_verts[f * 3 + 0];
            int vi1 = model->face_verts[f * 3 + 1];
            int vi2 = model->face_verts[f * 3 + 2];

            Vec3 v0 = model->vertices[vi0];
            Vec3 v1 = model->vertices[vi1];
            Vec3 v2 = model->vertices[vi2];

            Vec4 clip0 = mat4_mul_vec4(mvp, vec4_from_vec3(v0, 1.0f));
            Vec4 clip1 = mat4_mul_vec4(mvp, vec4_from_vec3(v1, 1.0f));
            Vec4 clip2 = mat4_mul_vec4(mvp, vec4_from_vec3(v2, 1.0f));

            // Get UVs
            bool has_uvs = model->texture != NULL && model->uvs != NULL;
            Vec2 uv0 = {0}, uv1 = {0}, uv2 = {0};
            if (has_uvs) {
                int ui0 = model->face_uvs[f * 3 + 0];
                int ui1 = model->face_uvs[f * 3 + 1];
                int ui2 = model->face_uvs[f * 3 + 2];
                if (ui0 >= 0 && ui0 < model->uv_count) uv0 = model->uvs[ui0];
                if (ui1 >= 0 && ui1 < model->uv_count) uv1 = model->uvs[ui1];
                if (ui2 >= 0 && ui2 < model->uv_count) uv2 = model->uvs[ui2];
            }

            // Near-plane clipping
            Vec4 clip_in[3] = { clip0, clip1, clip2 };
            Vec2 uv_in[3] = { uv0, uv1, uv2 };
            Vec4 clip_out[2][3];
            Vec2 uv_out[2][3];
            int tri_count;
            clip_triangle(clip_in, uv_in, has_uvs, 0.1f, clip_out, uv_out, &tri_count);

            for (int t = 0; t < tri_count; t++) {
                if (*chunk_count >= max_chunks) return;

                Vec3 ndc0 = vec4_perspective_divide(clip_out[t][0]);
                Vec3 ndc1 = vec4_perspective_divide(clip_out[t][1]);
                Vec3 ndc2 = vec4_perspective_divide(clip_out[t][2]);

                // Viewport transform
                float sx0 = (ndc0.x + 1.0f) * 0.5f * WINDOW_WIDTH;
                float sy0 = (1.0f - ndc0.y) * 0.5f * WINDOW_HEIGHT;
                float sz0 = (ndc0.z + 1.0f) * 0.5f;

                float sx1 = (ndc1.x + 1.0f) * 0.5f * WINDOW_WIDTH;
                float sy1 = (1.0f - ndc1.y) * 0.5f * WINDOW_HEIGHT;
                float sz1 = (ndc1.z + 1.0f) * 0.5f;

                float sx2 = (ndc2.x + 1.0f) * 0.5f * WINDOW_WIDTH;
                float sy2 = (1.0f - ndc2.y) * 0.5f * WINDOW_HEIGHT;
                float sz2 = (ndc2.z + 1.0f) * 0.5f;

                // Backface cull (cross_z < 0 means CW in screen space = front-facing)
                float edge1_x = sx1 - sx0;
                float edge1_y = sy1 - sy0;
                float edge2_x = sx2 - sx0;
                float edge2_y = sy2 - sy0;
                float cross_z = edge1_x * edge2_y - edge1_y * edge2_x;
                if (cross_z >= 0) continue;

                // Swap verts 1 and 2 so the rasterizer receives CCW winding
                Chunk *chunk = &chunks[*chunk_count];
                chunk->verts[0] = (ScreenVertex){ sx0, sy0, sz0 };
                chunk->verts[1] = (ScreenVertex){ sx2, sy2, sz2 };
                chunk->verts[2] = (ScreenVertex){ sx1, sy1, sz1 };
                chunk->depth_sort_key = minf(sz0, minf(sz1, sz2));

                if (has_uvs) {
                    chunk->type = CHUNK_TEXTURED;
                    chunk->textured.texture = model->texture;
                    chunk->textured.uvs[0] = uv_out[t][0];
                    chunk->textured.uvs[1] = uv_out[t][2];
                    chunk->textured.uvs[2] = uv_out[t][1];
                } else {
                    chunk->type = CHUNK_COLORED;
                    chunk->colored.color = face_color_from_index(f);
                }

                (*chunk_count)++;
            }
        }
    }
}

void scene_destroy(Scene *scene) {
    for (int i = 0; i < scene->model_count; i++) {
        Model *m = &scene->models[i];
        free(m->vertices);
        free(m->uvs);
        free(m->face_verts);
        free(m->face_uvs);
        if (m->texture) {
            free(m->texture->pixels);
            free(m->texture);
        }
    }
    memset(scene, 0, sizeof(Scene));
}
