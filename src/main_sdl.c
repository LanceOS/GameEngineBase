#include <stdlib.h>
#include <stdint.h>

#include <SDL2/SDL.h>

#include "../include/bgfx_renderer.h"
#include "../include/sdl_window.h"

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
    if (!sdl_window_native_handles(&game, &native_display, &native_window)) {
        sdl_window_shutdown(&game);
        return EXIT_FAILURE;
    }

    BgfxRenderer renderer = {0};
    if (!bgfx_renderer_init(&renderer, native_display, native_window, to_u16(game.width), to_u16(game.height))) {
        sdl_window_shutdown(&game);
        return EXIT_FAILURE;
    }

    while (sdl_window_is_running(&game)) {
        sdl_window_poll_events(&game);

        uint32_t resized_width = 0;
        uint32_t resized_height = 0;
        if (sdl_window_consume_resize(&game, &resized_width, &resized_height)) {
            bgfx_renderer_resize(&renderer, to_u16(resized_width), to_u16(resized_height));
        }

        bgfx_renderer_frame(&renderer);
        SDL_Delay(16);
    }

    bgfx_renderer_shutdown(&renderer);
    sdl_window_shutdown(&game);
    return EXIT_SUCCESS;
}
