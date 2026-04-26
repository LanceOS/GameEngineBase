#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <SDL2/SDL.h>

#include "bgfx_renderer.h"
#include "environment_3d.h"
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

    while (sdl_window_is_running(&game)) {
        sdl_window_poll_events(&game);

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
            environment_3d_frame(environment);
        }

        if (renderer_ok) {
            bgfx_renderer_frame(&renderer);
        }
        SDL_Delay(16);
    }

    if (environment_ok) {
        environment_3d_destroy(environment);
    }
    if (renderer_ok) {
        bgfx_renderer_shutdown(&renderer);
    }
    sdl_window_shutdown(&game);
    return EXIT_SUCCESS;
}
