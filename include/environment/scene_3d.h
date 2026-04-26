#ifndef SCENE_3D_H
#define SCENE_3D_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Scene3D Scene3D;

Scene3D *scene_3d_create(void);
void scene_3d_destroy(Scene3D *scene);
bool scene_3d_init(Scene3D *scene);
void scene_3d_frame(Scene3D *scene);

#ifdef __cplusplus
}
#endif

#endif
