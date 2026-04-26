#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "app/game_app.h"
#include "sdl_window.h"

static uint16_t to_u16(uint32_t value) {
    return value > UINT16_MAX ? UINT16_MAX : (uint16_t)value;
}

int main(void) {
    SDLGameState game;
    GameApp app;
    int previous_mouse_x = 0;
    int previous_mouse_y = 0;

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

    if (!game_app_init(&app, native_display, native_window, native_window_type, to_u16(game.width), to_u16(game.height))) {
        sdl_window_shutdown(&game);
        return EXIT_FAILURE;
    }

    const uint64_t performance_frequency = SDL_GetPerformanceFrequency();
    uint64_t previous_counter = SDL_GetPerformanceCounter();

    while (sdl_window_is_running(&game)) {
        GameFrameInput input = {0};
        const GameAppMode current_mode = game_app_mode(&app);
        SDL_Event event;

        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                case SDL_QUIT:
                    input.quit_requested = true;
                    break;

                case SDL_KEYDOWN:
                    if (event.key.repeat == 0) {
                        switch (event.key.keysym.sym) {
                            case SDLK_ESCAPE:
                                input.quit_requested = true;
                                break;

                            case SDLK_UP:
                                input.menu_up = true;
                                break;

                            case SDLK_DOWN:
                                input.menu_down = true;
                                break;

                            case SDLK_RETURN:
                            case SDLK_KP_ENTER:
                            case SDLK_SPACE:
                                input.menu_activate = true;
                                break;

                            default:
                                break;
                        }
                    }
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        input.mouse_click = true;
                    }
                    break;

                case SDL_MOUSEMOTION:
                    break;

                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        game.width = (uint32_t)event.window.data1;
                        game.height = (uint32_t)event.window.data2;
                        game.resized = true;
                    } else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                        input.quit_requested = true;
                    }
                    break;

                default:
                    break;
            }
        }

        const uint8_t *keyboard_state = SDL_GetKeyboardState(NULL);
        if (keyboard_state != NULL) {
            input.move_forward = keyboard_state[SDL_SCANCODE_W] != 0;
            input.move_backward = keyboard_state[SDL_SCANCODE_S] != 0;
            input.move_left = keyboard_state[SDL_SCANCODE_A] != 0;
            input.move_right = keyboard_state[SDL_SCANCODE_D] != 0;
            input.move_up = keyboard_state[SDL_SCANCODE_SPACE] != 0;
            input.move_down = keyboard_state[SDL_SCANCODE_LCTRL] != 0 || keyboard_state[SDL_SCANCODE_RCTRL] != 0;
        }

        int window_x = 0;
        int window_y = 0;
        if (game.window != NULL) {
            SDL_GetWindowPosition(game.window, &window_x, &window_y);
        }

        int mouse_x = 0;
        int mouse_y = 0;
        SDL_GetGlobalMouseState(&mouse_x, &mouse_y);

        const int local_mouse_x = mouse_x - window_x;
        const int local_mouse_y = mouse_y - window_y;
        input.mouse_x = local_mouse_x;
        input.mouse_y = local_mouse_y;
        input.mouse_moved = local_mouse_x != previous_mouse_x || local_mouse_y != previous_mouse_y;

        if (current_mode == GAME_APP_MODE_GAMEPLAY) {
            int mouse_dx = 0;
            int mouse_dy = 0;
            SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);
            input.mouse_delta_x = (float)mouse_dx;
            input.mouse_delta_y = (float)mouse_dy;
        }

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

        previous_mouse_x = local_mouse_x;
        previous_mouse_y = local_mouse_y;

        GameAppAction action = game_app_update(&app, &input, (float)delta_seconds);
        if (action == GAME_APP_ACTION_QUIT) {
            game.running = false;
        } else {
            if (action == GAME_APP_ACTION_STARTED_GAMEPLAY) {
                if (SDL_SetRelativeMouseMode(SDL_TRUE) != 0) {
                    fprintf(stderr, "Warning: SDL_SetRelativeMouseMode failed: %s\n", SDL_GetError());
                }
                SDL_ShowCursor(SDL_DISABLE);
            }

            game_app_render(&app);
        }
        SDL_Delay(16);
    }

    if (game_app_mode(&app) == GAME_APP_MODE_GAMEPLAY) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_ShowCursor(SDL_ENABLE);
    }

    game_app_destroy(&app);
    sdl_window_shutdown(&game);
    return EXIT_SUCCESS;
}
