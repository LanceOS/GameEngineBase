#include "environment/camera_3d.h"

#include <bx/math.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>

namespace {

static constexpr float kDefaultMoveSpeed = 4.5f;
static constexpr float kDefaultMouseSensitivity = 0.12f;
static constexpr float kDefaultX = 0.0f;
static constexpr float kDefaultY = 1.75f;
static constexpr float kDefaultZ = -4.0f;
static constexpr float kDefaultYaw = 90.0f;
static constexpr float kDefaultPitch = -23.5f;
static constexpr float kPitchLimit = 89.0f;
static constexpr float kDegToRad = 0.017453292519943295769f;

static float clampf(float value, float minimum, float maximum) {
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

static float wrap_degrees(float value) {
    float wrapped = std::fmod(value, 360.0f);
    if (wrapped < 0.0f) {
        wrapped += 360.0f;
    }
    return wrapped;
}

static void forward_from_angles(float yaw_degrees, float pitch_degrees, float out_forward[3]) {
    const float yaw_radians = yaw_degrees * kDegToRad;
    const float pitch_radians = pitch_degrees * kDegToRad;
    const float cos_pitch = std::cos(pitch_radians);

    out_forward[0] = cos_pitch * std::cos(yaw_radians);
    out_forward[1] = std::sin(pitch_radians);
    out_forward[2] = cos_pitch * std::sin(yaw_radians);
}

static float vector_length(const float value[3]) {
    return std::sqrt(value[0] * value[0] + value[1] * value[1] + value[2] * value[2]);
}

static bool normalize_vector(float value[3]) {
    const float length = vector_length(value);
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

static void rebuild_view(float view[16], const float position[3], float yaw_degrees, float pitch_degrees) {
    float forward[3];
    forward_from_angles(yaw_degrees, pitch_degrees, forward);

    const bx::Vec3 eye(position[0], position[1], position[2]);
    const bx::Vec3 at(position[0] + forward[0], position[1] + forward[1], position[2] + forward[2]);
    const bx::Vec3 up(0.0f, 1.0f, 0.0f);

    bx::mtxLookAt(view, eye, at, up);
}

} // namespace

struct Camera3D {
    float position[3];
    float yaw_degrees;
    float pitch_degrees;
    float move_speed;
    float mouse_sensitivity;
    float view[16];
};

extern "C" {

Camera3D *camera_3d_create(void) {
    Camera3D *camera = new (std::nothrow) Camera3D();
    if (camera == nullptr) {
        return nullptr;
    }

    camera_3d_reset(camera, kDefaultX, kDefaultY, kDefaultZ, kDefaultYaw, kDefaultPitch);
    camera_3d_set_move_speed(camera, kDefaultMoveSpeed);
    camera_3d_set_mouse_sensitivity(camera, kDefaultMouseSensitivity);
    return camera;
}

void camera_3d_destroy(Camera3D *camera) {
    delete camera;
}

bool camera_3d_reset(Camera3D *camera, float x, float y, float z, float yaw_degrees, float pitch_degrees) {
    if (camera == nullptr) {
        return false;
    }

    camera->position[0] = x;
    camera->position[1] = y;
    camera->position[2] = z;
    camera->yaw_degrees = wrap_degrees(yaw_degrees);
    camera->pitch_degrees = clampf(pitch_degrees, -kPitchLimit, kPitchLimit);
    rebuild_view(camera->view, camera->position, camera->yaw_degrees, camera->pitch_degrees);
    return true;
}

void camera_3d_set_move_speed(Camera3D *camera, float move_speed) {
    if (camera == nullptr) {
        return;
    }

    camera->move_speed = move_speed < 0.0f ? 0.0f : move_speed;
}

void camera_3d_set_mouse_sensitivity(Camera3D *camera, float mouse_sensitivity) {
    if (camera == nullptr) {
        return;
    }

    camera->mouse_sensitivity = mouse_sensitivity < 0.0f ? 0.0f : mouse_sensitivity;
}

void camera_3d_update(Camera3D *camera, const CameraInputState *input, float delta_seconds) {
    if (camera == nullptr) {
        return;
    }

    const float mouse_dx = input != nullptr ? input->mouse_delta_x : 0.0f;
    const float mouse_dy = input != nullptr ? input->mouse_delta_y : 0.0f;

    camera->yaw_degrees = wrap_degrees(camera->yaw_degrees + mouse_dx * camera->mouse_sensitivity);
    camera->pitch_degrees = clampf(camera->pitch_degrees - mouse_dy * camera->mouse_sensitivity, -kPitchLimit, kPitchLimit);

    float forward[3];
    forward_from_angles(camera->yaw_degrees, camera->pitch_degrees, forward);

    float flat_forward[3] = {forward[0], 0.0f, forward[2]};
    if (!normalize_vector(flat_forward)) {
        flat_forward[0] = 0.0f;
        flat_forward[1] = 0.0f;
        flat_forward[2] = 1.0f;
    }

    const float right[3] = {flat_forward[2], 0.0f, -flat_forward[0]};
    const float movement = camera->move_speed * (delta_seconds < 0.0f ? 0.0f : delta_seconds);

    if (input != nullptr) {
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
    return camera != nullptr ? camera->view : nullptr;
}

void camera_3d_get_position(const Camera3D *camera, float *x, float *y, float *z) {
    if (camera == nullptr) {
        return;
    }

    if (x != nullptr) {
        *x = camera->position[0];
    }
    if (y != nullptr) {
        *y = camera->position[1];
    }
    if (z != nullptr) {
        *z = camera->position[2];
    }
}

void camera_3d_get_orientation(const Camera3D *camera, float *yaw_degrees, float *pitch_degrees) {
    if (camera == nullptr) {
        return;
    }

    if (yaw_degrees != nullptr) {
        *yaw_degrees = camera->yaw_degrees;
    }
    if (pitch_degrees != nullptr) {
        *pitch_degrees = camera->pitch_degrees;
    }
}

} // extern "C"
