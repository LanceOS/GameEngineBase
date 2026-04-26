#include "environment/environment_3d.h"

#include "environment/camera_3d.h"
#include "environment/scene_3d.h"

#include <bgfx/c99/bgfx.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static const uint16_t kViewId = 0;
static const uint32_t kClearColor = 0x1a1f2cff;
static const float kDegToRad = 0.017453292519943295769f;

struct Environment3D {
    uint16_t width;
    uint16_t height;
    bool homogeneous_depth;
    bool initialized;
    Scene3D *scene;
    float view[16];
    float proj[16];
};

static uint16_t clamp_dimension(uint16_t value) {
    return value == 0 ? 1 : value;
}

static void set_identity(float matrix[16]) {
    size_t i;
    for (i = 0; i < 16; ++i) {
        matrix[i] = 0.0f;
    }

    matrix[0] = 1.0f;
    matrix[5] = 1.0f;
    matrix[10] = 1.0f;
    matrix[15] = 1.0f;
}

static void vec3_sub(float out[3], const float a[3], const float b[3]) {
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

static float vec3_dot(const float a[3], const float b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void vec3_cross(float out[3], const float a[3], const float b[3]) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

static float vec3_length(const float value[3]) {
    return sqrtf(value[0] * value[0] + value[1] * value[1] + value[2] * value[2]);
}

static bool normalize_vector(float value[3]) {
    const float length = vec3_length(value);
    if (length <= 0.0f) {
        return false;
    }

    value[0] /= length;
    value[1] /= length;
    value[2] /= length;
    return true;
}

static void build_look_at(float view[16], const float eye[3], const float target[3], const float up[3]) {
    float forward[3];
    float right[3];
    float camera_up[3];
    const float fallback_up[3] = {0.0f, 0.0f, 1.0f};

    vec3_sub(forward, target, eye);
    if (!normalize_vector(forward)) {
        set_identity(view);
        return;
    }

    vec3_cross(right, forward, up);
    if (!normalize_vector(right)) {
        vec3_cross(right, forward, fallback_up);
        if (!normalize_vector(right)) {
            set_identity(view);
            return;
        }
    }

    vec3_cross(camera_up, right, forward);

    view[0] = right[0];
    view[1] = camera_up[0];
    view[2] = -forward[0];
    view[3] = 0.0f;

    view[4] = right[1];
    view[5] = camera_up[1];
    view[6] = -forward[1];
    view[7] = 0.0f;

    view[8] = right[2];
    view[9] = camera_up[2];
    view[10] = -forward[2];
    view[11] = 0.0f;

    view[12] = -vec3_dot(right, eye);
    view[13] = -vec3_dot(camera_up, eye);
    view[14] = vec3_dot(forward, eye);
    view[15] = 1.0f;
}

static void build_default_view(float view[16]) {
    const float eye[3] = {0.0f, 1.75f, -4.0f};
    const float target[3] = {0.0f, 0.0f, 0.0f};
    const float up[3] = {0.0f, 1.0f, 0.0f};

    build_look_at(view, eye, target, up);
}

static void build_projection(float proj[16], float fov_y_degrees, float aspect_ratio, float near_plane, float far_plane, bool homogeneous_depth) {
    size_t i;
    const float fov_radians = fov_y_degrees * kDegToRad;
    const float f = 1.0f / tanf(fov_radians * 0.5f);

    for (i = 0; i < 16; ++i) {
        proj[i] = 0.0f;
    }

    proj[0] = f / aspect_ratio;
    proj[5] = f;
    proj[11] = -1.0f;

    if (homogeneous_depth) {
        proj[10] = (far_plane + near_plane) / (near_plane - far_plane);
        proj[14] = (2.0f * far_plane * near_plane) / (near_plane - far_plane);
    } else {
        proj[10] = far_plane / (near_plane - far_plane);
        proj[14] = (near_plane * far_plane) / (near_plane - far_plane);
    }
}

static void apply_view_state(const Environment3D *environment, const float *view_matrix) {
    bgfx_set_view_clear(kViewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, kClearColor, 1.0f, 0);
    bgfx_set_view_rect(kViewId, 0, 0, environment->width, environment->height);
    bgfx_set_view_transform(kViewId, view_matrix, environment->proj);
}

static void rebuild_matrices(Environment3D *environment) {
    const float aspect = (float)environment->width / (float)environment->height;

    build_default_view(environment->view);
    build_projection(environment->proj, 60.0f, aspect, 0.1f, 100.0f, environment->homogeneous_depth);
}

Environment3D *environment_3d_create(void) {
    Environment3D *environment = (Environment3D *)calloc(1, sizeof(*environment));
    if (environment == NULL) {
        return NULL;
    }

    build_default_view(environment->view);
    set_identity(environment->proj);
    return environment;
}

void environment_3d_destroy(Environment3D *environment) {
    if (environment == NULL) {
        return;
    }

    scene_3d_destroy(environment->scene);
    free(environment);
}

bool environment_3d_init(Environment3D *environment, uint16_t width, uint16_t height) {
    const bgfx_caps_t *caps;

    if (environment == NULL) {
        return false;
    }

    caps = bgfx_get_caps();
    if (caps == NULL) {
        fputs("environment_3d_init failed: bgfx caps unavailable.\n", stderr);
        return false;
    }

    scene_3d_destroy(environment->scene);
    environment->scene = NULL;

    environment->width = clamp_dimension(width);
    environment->height = clamp_dimension(height);
    environment->homogeneous_depth = caps->homogeneousDepth;

    environment->scene = scene_3d_create();
    if (environment->scene == NULL || !scene_3d_init(environment->scene)) {
        fputs("environment_3d_init failed: scene unavailable.\n", stderr);
        scene_3d_destroy(environment->scene);
        environment->scene = NULL;
        environment->initialized = false;
        return false;
    }

    environment->initialized = true;
    rebuild_matrices(environment);
    apply_view_state(environment, environment->view);
    return true;
}

void environment_3d_resize(Environment3D *environment, uint16_t width, uint16_t height) {
    if (environment == NULL || !environment->initialized) {
        return;
    }

    environment->width = clamp_dimension(width);
    environment->height = clamp_dimension(height);
    rebuild_matrices(environment);
    apply_view_state(environment, environment->view);
}

void environment_3d_frame(Environment3D *environment, const Camera3D *camera) {
    const float *view_matrix;

    if (environment == NULL || !environment->initialized) {
        return;
    }

    view_matrix = camera_3d_view_matrix(camera);
    if (view_matrix == NULL) {
        view_matrix = environment->view;
    }

    apply_view_state(environment, view_matrix);
    scene_3d_frame(environment->scene);
    bgfx_touch(kViewId);
}
