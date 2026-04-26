#ifndef SDL_WINDOW_H
#define SDL_WINDOW_H

#include <SDL.h>

#include <stdbool.h>
#include <stdint.h>

typedef struct SDLGameState {
	SDL_Window *window;
	bool running;
	uint32_t width;
	uint32_t height;
	bool resized;
} SDLGameState;

bool sdl_window_init(SDLGameState *game, uint32_t width, uint32_t height, const char *title);
void sdl_window_poll_events(SDLGameState *game);
bool sdl_window_consume_resize(SDLGameState *game, uint32_t *new_width, uint32_t *new_height);
bool sdl_window_native_handles(SDLGameState *game, void **native_display, void **native_window, uint32_t *native_window_type);
bool sdl_window_is_running(const SDLGameState *game);
void sdl_window_shutdown(SDLGameState *game);

#endif
