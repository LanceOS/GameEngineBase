#ifndef ENVIRONMENT_3D_H
#define ENVIRONMENT_3D_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct Camera3D Camera3D;

typedef struct Environment3D Environment3D;

Environment3D *environment_3d_create(void);
void environment_3d_destroy(Environment3D *environment);
bool environment_3d_init(Environment3D *environment, uint16_t width, uint16_t height);
void environment_3d_resize(Environment3D *environment, uint16_t width, uint16_t height);
void environment_3d_frame(Environment3D *environment, const Camera3D *camera);

#ifdef __cplusplus
}
#endif

#endif
