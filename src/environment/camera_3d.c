#include "environment/camera_3d.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>

static const float kDefaultMoveSpeed = 4.5f;
static const float kDefaultMouseSensitivity = 0.12f;
static const float kDefaultX = 0.0f;
static const float kDefaultY = 1.75f;
static const float kDefaultZ = -4.0f;
static const float kDefaultYaw = 90.0f;
static const float kDefaultPitch = -23.5f;
static const float kPitchLimit = 89.0f;
static const float kDegToRad = 0.017453292519943295769f;

struct Camera3D {
    float position[3];
    float yaw_degrees;
    float pitch_degrees;
    float move_speed;
    float mouse_sensitivity;
    float view[16];
};

static float clampf_local(float value, float minimum, float maximum) {
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

static float wrap_degrees(float value) {
    float wrapped = fmodf(value, 360.0f);
    if (wrapped < 0.0f) {
        wrapped += 360.0f;
    }
    return wrapped;
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

static void forward_from_angles(float yaw_degrees, float pitch_degrees, float out_forward[3]) {
    const float yaw_radians = yaw_degrees * kDegToRad;
    const float pitch_radians = pitch_degrees * kDegToRad;
    const float cos_pitch = cosf(pitch_radians);

    out_forward[0] = cos_pitch * cosf(yaw_radians);
    out_forward[1] = sinf(pitch_radians);
    out_forward[2] = cos_pitch * sinf(yaw_radians);
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

static void add_scaled(float position[3], const float direction[3], float scale) {
    position[0] += direction[0] * scale;
    position[1] += direction[1] * scale;
    position[2] += direction[2] * scale;
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

    vec3_cross(right, up, forward);
    if (!normalize_vector(right)) {
        vec3_cross(right, fallback_up, forward);
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

static void rebuild_view(float view[16], const float position[3], float yaw_degrees, float pitch_degrees) {
    float forward[3];
    float target[3];
    const float up[3] = {0.0f, 1.0f, 0.0f};

    forward_from_angles(yaw_degrees, pitch_degrees, forward);
    target[0] = position[0] + forward[0];
    target[1] = position[1] + forward[1];
    target[2] = position[2] + forward[2];
    build_look_at(view, position, target, up);
}

Camera3D *camera_3d_create(void) {
    Camera3D *camera = (Camera3D *)calloc(1, sizeof(*camera));
    if (camera == NULL) {
        return NULL;
    }

    camera_3d_reset(camera, kDefaultX, kDefaultY, kDefaultZ, kDefaultYaw, kDefaultPitch);
    camera_3d_set_move_speed(camera, kDefaultMoveSpeed);
    camera_3d_set_mouse_sensitivity(camera, kDefaultMouseSensitivity);
    return camera;
}

void camera_3d_destroy(Camera3D *camera) {
    free(camera);
}

bool camera_3d_reset(Camera3D *camera, float x, float y, float z, float yaw_degrees, float pitch_degrees) {
    if (camera == NULL) {
        return false;
    }

    camera->position[0] = x;
    camera->position[1] = y;
    camera->position[2] = z;
    camera->yaw_degrees = wrap_degrees(yaw_degrees);
    camera->pitch_degrees = clampf_local(pitch_degrees, -kPitchLimit, kPitchLimit);
    rebuild_view(camera->view, camera->position, camera->yaw_degrees, camera->pitch_degrees);
    return true;
}

void camera_3d_set_move_speed(Camera3D *camera, float move_speed) {
    if (camera == NULL) {
        return;
    }

    camera->move_speed = move_speed < 0.0f ? 0.0f : move_speed;
}

void camera_3d_set_mouse_sensitivity(Camera3D *camera, float mouse_sensitivity) {
    if (camera == NULL) {
        return;
    }

    camera->mouse_sensitivity = mouse_sensitivity < 0.0f ? 0.0f : mouse_sensitivity;
}

void camera_3d_update(Camera3D *camera, const CameraInputState *input, float delta_seconds) {
    float forward[3];
    float flat_forward[3];
    float right[3];
    float movement;
    const float mouse_dx = input != NULL ? input->mouse_delta_x : 0.0f;
    const float mouse_dy = input != NULL ? input->mouse_delta_y : 0.0f;

    if (camera == NULL) {
        return;
    }

    camera->yaw_degrees = wrap_degrees(camera->yaw_degrees - mouse_dx * camera->mouse_sensitivity);
    camera->pitch_degrees = clampf_local(camera->pitch_degrees - mouse_dy * camera->mouse_sensitivity, -kPitchLimit, kPitchLimit);

    forward_from_angles(camera->yaw_degrees, camera->pitch_degrees, forward);
    flat_forward[0] = forward[0];
    flat_forward[1] = 0.0f;
    flat_forward[2] = forward[2];

    if (!normalize_vector(flat_forward)) {
        flat_forward[0] = 0.0f;
        flat_forward[1] = 0.0f;
        flat_forward[2] = 1.0f;
    }

    right[0] = flat_forward[2];
    right[1] = 0.0f;
    right[2] = -flat_forward[0];

    movement = camera->move_speed * (delta_seconds < 0.0f ? 0.0f : delta_seconds);

    if (input != NULL) {
        if (input->move_forward) {
            add_scaled(camera->position, flat_forward, movement);
        }
        if (input->move_backward) {
            add_scaled(camera->position, flat_forward, -movement);
        }
        if (input->move_right) {
            add_scaled(camera->position, right, movement);
        }
        if (input->move_left) {
            add_scaled(camera->position, right, -movement);
        }
        if (input->move_up) {
            camera->position[1] += movement;
        }
        if (input->move_down) {
            camera->position[1] -= movement;
        }
    }

    rebuild_view(camera->view, camera->position, camera->yaw_degrees, camera->pitch_degrees);
}

const float *camera_3d_view_matrix(const Camera3D *camera) {
    return camera != NULL ? camera->view : NULL;
}

void camera_3d_get_position(const Camera3D *camera, float *x, float *y, float *z) {
    if (camera == NULL) {
        return;
    }

    if (x != NULL) {
        *x = camera->position[0];
    }
    if (y != NULL) {
        *y = camera->position[1];
    }
    if (z != NULL) {
        *z = camera->position[2];
    }
}

void camera_3d_get_orientation(const Camera3D *camera, float *yaw_degrees, float *pitch_degrees) {
    if (camera == NULL) {
        return;
    }

    if (yaw_degrees != NULL) {
        *yaw_degrees = camera->yaw_degrees;
    }
    if (pitch_degrees != NULL) {
        *pitch_degrees = camera->pitch_degrees;
    }
}
