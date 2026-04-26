#include "../include/sdl_window.h"

#include <SDL2/SDL_syswm.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

bool sdl_window_init(SDLGameState *game, uint32_t width, uint32_t height, const char *title) {
    if (game == NULL) {
        return false;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    memset(game, 0, sizeof(*game));

    const uint32_t window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    game->window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        (int)width,
        (int)height,
        window_flags);

    if (game->window == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    game->running = true;
    game->width = width;
    game->height = height;
    game->resized = false;
    return true;
}

void sdl_window_poll_events(SDLGameState *game) {
    if (game == NULL) {
        return;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
            case SDL_QUIT:
                game->running = false;
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    game->running = false;
                }
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    game->width = (uint32_t)event.window.data1;
                    game->height = (uint32_t)event.window.data2;
                    game->resized = true;
                }
                break;

            default:
                break;
        }
    }
}

bool sdl_window_consume_resize(SDLGameState *game, uint32_t *new_width, uint32_t *new_height) {
    if (game == NULL || !game->resized) {
        return false;
    }

    if (new_width != NULL) {
        *new_width = game->width;
    }
    if (new_height != NULL) {
        *new_height = game->height;
    }

    game->resized = false;
    return true;
}

bool sdl_window_native_handles(SDLGameState *game, void **native_display, void **native_window) {
    if (native_display != NULL) {
        *native_display = NULL;
    }
    if (native_window != NULL) {
        *native_window = NULL;
    }

    if (game == NULL || game->window == NULL || native_window == NULL) {
        return false;
    }

    SDL_SysWMinfo wm_info;
    SDL_VERSION(&wm_info.version);

    if (SDL_GetWindowWMInfo(game->window, &wm_info) == SDL_FALSE) {
        fprintf(stderr, "SDL_GetWindowWMInfo failed: %s\n", SDL_GetError());
        return false;
    }

    switch (wm_info.subsystem) {
        case SDL_SYSWM_WINDOWS:
            *native_window = wm_info.info.win.window;
            return true;

        case SDL_SYSWM_X11:
            if (native_display != NULL) {
                *native_display = wm_info.info.x11.display;
            }
            *native_window = (void *)(uintptr_t)wm_info.info.x11.window;
            return true;

        case SDL_SYSWM_WAYLAND:
            if (native_display != NULL) {
                *native_display = wm_info.info.wl.display;
            }
            *native_window = wm_info.info.wl.surface;
            return true;

        case SDL_SYSWM_COCOA:
            *native_window = wm_info.info.cocoa.window;
            return true;

        default:
            fprintf(stderr, "Unsupported SDL subsystem (%d) for bgfx platform data.\n", (int)wm_info.subsystem);
            return false;
    }
}

bool sdl_window_is_running(const SDLGameState *game) {
    return game != NULL && game->running;
}

void sdl_window_shutdown(SDLGameState *game) {
    if (game == NULL) {
        return;
    }

    if (game->window != NULL) {
        SDL_DestroyWindow(game->window);
        game->window = NULL;
    }

    SDL_Quit();
    game->running = false;
}
