#ifndef CONTROLS_H
#define CONTROLS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declare SDLGameState to avoid pulling SDL headers into this public header. */
struct SDLGameState;

#include "app/game_app.h" /* defines GameFrameInput */

typedef struct Controls Controls;

/* Create/destroy */
Controls *controls_create_for_sdl(struct SDLGameState *sdl_state);
Controls *controls_create_for_x11(void *display, unsigned long window, int screen);
void controls_destroy(Controls *c);

/* Per-frame event processing */
void controls_poll_events(Controls *c);
void controls_get_frame_input(Controls *c, GameFrameInput *out, bool gameplay_mode);

/* Helpers */
bool controls_set_relative_mouse_mode(Controls *c, bool enable);
void controls_set_cursor_visible(Controls *c, bool visible);
/* If a resize occurred, fill width/height and return true; otherwise return false. */
bool controls_consume_resize(Controls *c, uint16_t *width, uint16_t *height);

#ifdef __cplusplus
}
#endif

#endif
