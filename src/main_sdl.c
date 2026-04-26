#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <SDL.h>
#include "environment/camera_3d.h"
#include "renderer/bgfx_renderer.h"
#include "environment/environment_3d.h"
#include "sdl_window.h"

static uint16_t to_u16(uint32_t value) {
    return value > UINT16_MAX ? UINT16_MAX : (uint16_t)value;
}

int main(void) {
    SDLGameState game;

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

    BgfxRenderer renderer = {0};
    bool renderer_ok = false;
    if (bgfx_renderer_init(&renderer, native_display, native_window, native_window_type, to_u16(game.width), to_u16(game.height))) {
        renderer_ok = true;
    } else {
        fprintf(stderr, "Warning: bgfx_renderer_init failed; running without GPU renderer.\n");
        /* Continue running the event loop so the app remains responsive in headless environments. */
    }

    Camera3D *camera = NULL;
    bool camera_ok = false;
    if (renderer_ok) {
        camera = camera_3d_create();
        if (camera != NULL) {
            camera_ok = true;
            if (SDL_SetRelativeMouseMode(SDL_TRUE) != 0) {
                fprintf(stderr, "Warning: SDL_SetRelativeMouseMode failed: %s\n", SDL_GetError());
            }
            SDL_ShowCursor(SDL_DISABLE);
        } else {
            fprintf(stderr, "Warning: camera_3d_create failed; running with the environment fallback view.\n");
        }
    }

    Environment3D *environment = NULL;
    bool environment_ok = false;
    if (renderer_ok) {
        environment = environment_3d_create();
        if (environment != NULL && environment_3d_init(environment, to_u16(game.width), to_u16(game.height))) {
            environment_ok = true;
        } else {
            fprintf(stderr, "Warning: environment_3d_init failed; running without empty 3D environment.\n");
            environment_3d_destroy(environment);
            environment = NULL;
        }
    }

    const uint64_t performance_frequency = SDL_GetPerformanceFrequency();
    uint64_t previous_counter = SDL_GetPerformanceCounter();

    while (sdl_window_is_running(&game)) {
        sdl_window_poll_events(&game);

        uint64_t current_counter = SDL_GetPerformanceCounter();
        double delta_seconds = 0.0;
        if (performance_frequency > 0) {
            delta_seconds = (double)(current_counter - previous_counter) / (double)performance_frequency;
        }
        previous_counter = current_counter;
        if (delta_seconds > 0.25) {
            delta_seconds = 0.25;
        }

        CameraInputState camera_input = {0};
        const uint8_t *keyboard_state = SDL_GetKeyboardState(NULL);
        if (keyboard_state != NULL) {
            camera_input.move_forward = keyboard_state[SDL_SCANCODE_W] != 0;
            camera_input.move_backward = keyboard_state[SDL_SCANCODE_S] != 0;
            camera_input.move_left = keyboard_state[SDL_SCANCODE_A] != 0;
            camera_input.move_right = keyboard_state[SDL_SCANCODE_D] != 0;
            camera_input.move_up = keyboard_state[SDL_SCANCODE_SPACE] != 0;
            camera_input.move_down = keyboard_state[SDL_SCANCODE_LCTRL] != 0 || keyboard_state[SDL_SCANCODE_RCTRL] != 0;
        }

        int mouse_dx = 0;
        int mouse_dy = 0;
        SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);
        camera_input.mouse_delta_x = (float)mouse_dx;
        camera_input.mouse_delta_y = (float)mouse_dy;

        if (camera_ok) {
            camera_3d_update(camera, &camera_input, (float)delta_seconds);
        }

        uint32_t resized_width = 0;
        uint32_t resized_height = 0;
        if (sdl_window_consume_resize(&game, &resized_width, &resized_height)) {
            if (renderer_ok) {
                bgfx_renderer_resize(&renderer, to_u16(resized_width), to_u16(resized_height));
            }
            if (environment_ok) {
                environment_3d_resize(environment, to_u16(resized_width), to_u16(resized_height));
            }
        }

        if (environment_ok) {
            environment_3d_frame(environment, camera_ok ? camera : NULL);
        }

        if (renderer_ok) {
            bgfx_renderer_frame(&renderer);
        }
        SDL_Delay(16);
    }

    if (environment_ok) {
        environment_3d_destroy(environment);
    }
    if (camera_ok) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_ShowCursor(SDL_ENABLE);
        camera_3d_destroy(camera);
    }
    if (renderer_ok) {
        bgfx_renderer_shutdown(&renderer);
    }
    sdl_window_shutdown(&game);
    return EXIT_SUCCESS;
}
