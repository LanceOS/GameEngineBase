#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "app/game_app.h"
#include "sdl_window.h"
#include "controls/controls.h"

static uint16_t to_u16(uint32_t value) {
    return value > UINT16_MAX ? UINT16_MAX : (uint16_t)value;
}

int main(void) {
    SDLGameState game;
    GameApp app;
    Controls *controls = NULL;

    SDL_SetMainReady();

    if (!sdl_window_init(&game, 800, 600, "Game")) {
        return EXIT_FAILURE;
    }

    void *native_display = NULL;
    void *native_window = NULL;
    uint32_t native_window_type = 0;
    if (!sdl_window_native_handles(&game, &native_display, &native_window, &native_window_type)) {
        sdl_window_shutdown(&game);
        return EXIT_FAILURE;
    }

    controls = controls_create_for_sdl(&game);
    if (controls == NULL) {
        sdl_window_shutdown(&game);
        return EXIT_FAILURE;
    }

    if (!game_app_init(&app, native_display, native_window, native_window_type, to_u16(game.width), to_u16(game.height))) {
        sdl_window_shutdown(&game);
        return EXIT_FAILURE;
    }

    const uint64_t performance_frequency = SDL_GetPerformanceFrequency();
    uint64_t previous_counter = SDL_GetPerformanceCounter();

    while (sdl_window_is_running(&game)) {
        GameFrameInput input = {0};
        const GameAppMode current_mode = game_app_mode(&app);

        controls_poll_events(controls);
        controls_get_frame_input(controls, &input, current_mode == GAME_APP_MODE_GAMEPLAY);

        uint64_t current_counter = SDL_GetPerformanceCounter();
        double delta_seconds = 0.0;
        if (performance_frequency > 0) {
            delta_seconds = (double)(current_counter - previous_counter) / (double)performance_frequency;
        }
        previous_counter = current_counter;
        if (delta_seconds > 0.25) {
            delta_seconds = 0.25;
        }

        uint32_t resized_width = 0;
        uint32_t resized_height = 0;
        if (sdl_window_consume_resize(&game, &resized_width, &resized_height)) {
            game_app_resize(&app, to_u16(resized_width), to_u16(resized_height));
        }

        GameAppAction action = game_app_update(&app, &input, (float)delta_seconds);
        if (action == GAME_APP_ACTION_QUIT) {
            game.running = false;
        } else {
            if (action == GAME_APP_ACTION_STARTED_GAMEPLAY) {
                if (!controls_set_relative_mouse_mode(controls, true)) {
                    fprintf(stderr, "Warning: controls_set_relative_mouse_mode failed\n");
                }
                controls_set_cursor_visible(controls, false);
            }

            game_app_render(&app);
        }
        SDL_Delay(16);
    }

    if (game_app_mode(&app) == GAME_APP_MODE_GAMEPLAY) {
        controls_set_relative_mouse_mode(controls, false);
        controls_set_cursor_visible(controls, true);
    }

    game_app_destroy(&app);
    controls_destroy(controls);
    sdl_window_shutdown(&game);
    return EXIT_SUCCESS;
}
