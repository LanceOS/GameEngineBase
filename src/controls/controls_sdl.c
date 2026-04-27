#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <SDL.h>

#include "controls/controls.h"
#include "sdl_window.h"

struct Controls {
    SDLGameState *game;
    int previous_mouse_x;
    int previous_mouse_y;
    bool quit_requested;
    bool menu_up;
    bool menu_down;
    bool menu_activate;
    bool mouse_click;
    bool relative_mode_enabled;
};

Controls *controls_create_for_sdl(struct SDLGameState *sdl_state) {
    if (sdl_state == NULL) {
        return NULL;
    }
    Controls *c = (Controls *)malloc(sizeof(Controls));
    if (c == NULL) {
        return NULL;
    }
    memset(c, 0, sizeof(*c));
    c->game = sdl_state;
    c->previous_mouse_x = 0;
    c->previous_mouse_y = 0;
    return c;
}

void controls_destroy(Controls *c) {
    if (c == NULL) return;
    free(c);
}

void controls_poll_events(Controls *c) {
    if (c == NULL) return;
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
            case SDL_QUIT:
                c->quit_requested = true;
                break;

            case SDL_KEYDOWN:
                if (event.key.repeat == 0) {
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                            c->quit_requested = true;
                            break;

                        case SDLK_UP:
                            c->menu_up = true;
                            break;

                        case SDLK_DOWN:
                            c->menu_down = true;
                            break;

                        case SDLK_RETURN:
                        case SDLK_KP_ENTER:
                        case SDLK_SPACE:
                            c->menu_activate = true;
                            break;

                        default:
                            break;
                    }
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    c->mouse_click = true;
                }
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    if (c->game != NULL) {
                        c->game->width = (uint32_t)event.window.data1;
                        c->game->height = (uint32_t)event.window.data2;
                        c->game->resized = true;
                    }
                } else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    c->quit_requested = true;
                }
                break;

            default:
                break;
        }
    }
}

void controls_get_frame_input(Controls *c, GameFrameInput *out, bool gameplay_mode) {
    if (c == NULL || out == NULL) return;
    memset(out, 0, sizeof(*out));

    out->quit_requested = c->quit_requested;
    out->menu_up = c->menu_up;
    out->menu_down = c->menu_down;
    out->menu_activate = c->menu_activate;
    out->mouse_click = c->mouse_click;

    const uint8_t *keyboard_state = SDL_GetKeyboardState(NULL);
    if (keyboard_state != NULL) {
        out->move_forward = keyboard_state[SDL_SCANCODE_W] != 0;
        out->move_backward = keyboard_state[SDL_SCANCODE_S] != 0;
        out->move_left = keyboard_state[SDL_SCANCODE_A] != 0;
        out->move_right = keyboard_state[SDL_SCANCODE_D] != 0;
        out->move_up = keyboard_state[SDL_SCANCODE_SPACE] != 0;
        out->move_down = keyboard_state[SDL_SCANCODE_LCTRL] != 0 || keyboard_state[SDL_SCANCODE_RCTRL] != 0;
    }

    int window_x = 0;
    int window_y = 0;
    if (c->game != NULL && c->game->window != NULL) {
        SDL_GetWindowPosition(c->game->window, &window_x, &window_y);
    }

    int mouse_x = 0;
    int mouse_y = 0;
    SDL_GetGlobalMouseState(&mouse_x, &mouse_y);

    const int local_mouse_x = mouse_x - window_x;
    const int local_mouse_y = mouse_y - window_y;
    out->mouse_x = local_mouse_x;
    out->mouse_y = local_mouse_y;
    out->mouse_moved = local_mouse_x != c->previous_mouse_x || local_mouse_y != c->previous_mouse_y;

    if (gameplay_mode) {
        int mouse_dx = 0;
        int mouse_dy = 0;
        SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);
        out->mouse_delta_x = (float)mouse_dx;
        out->mouse_delta_y = (float)mouse_dy;
    } else {
        out->mouse_delta_x = 0.0f;
        out->mouse_delta_y = 0.0f;
    }

    c->previous_mouse_x = local_mouse_x;
    c->previous_mouse_y = local_mouse_y;

    /* Clear edge-state flags after they're consumed */
    c->mouse_click = false;
    c->menu_up = false;
    c->menu_down = false;
    c->menu_activate = false;
    c->quit_requested = false;
}

bool controls_set_relative_mouse_mode(Controls *c, bool enable) {
    (void)c;
    return SDL_SetRelativeMouseMode(enable ? SDL_TRUE : SDL_FALSE) == 0;
}

void controls_set_cursor_visible(Controls *c, bool visible) {
    (void)c;
    SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

bool controls_consume_resize(Controls *c, uint16_t *width, uint16_t *height) {
    /* SDL resize is handled via sdl_window_consume_resize in main_sdl.c; expose a no-op here. */
    (void)c; (void)width; (void)height;
    return false;
}
