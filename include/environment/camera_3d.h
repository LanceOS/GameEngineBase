#ifndef CAMERA_3D_H
#define CAMERA_3D_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Camera3D Camera3D;

typedef struct CameraInputState {
    bool move_forward;
    bool move_backward;
    bool move_left;
    bool move_right;
    bool move_up;
    bool move_down;
    float mouse_delta_x;
    float mouse_delta_y;
} CameraInputState;

Camera3D *camera_3d_create(void);
void camera_3d_destroy(Camera3D *camera);
bool camera_3d_reset(Camera3D *camera, float x, float y, float z, float yaw_degrees, float pitch_degrees);
void camera_3d_set_move_speed(Camera3D *camera, float move_speed);
void camera_3d_set_mouse_sensitivity(Camera3D *camera, float mouse_sensitivity);
void camera_3d_update(Camera3D *camera, const CameraInputState *input, float delta_seconds);
const float *camera_3d_view_matrix(const Camera3D *camera);
void camera_3d_get_position(const Camera3D *camera, float *x, float *y, float *z);
void camera_3d_get_orientation(const Camera3D *camera, float *yaw_degrees, float *pitch_degrees);

#ifdef __cplusplus
}
#endif

#endif
