#include "app/game_app.h"

#include "environment/camera_3d.h"
#include "environment/environment_3d.h"
#include "renderer/bgfx_renderer.h"

#include <bgfx/c99/bgfx.h>

#include <stdio.h>
#include <string.h>

static bool start_gameplay(GameApp *app) {
    if (app == NULL || app->mode == GAME_APP_MODE_GAMEPLAY) {
        return true;
    }

    app->camera = camera_3d_create();
    if (app->camera == NULL) {
        fputs("game_app: failed to create camera.\n", stderr);
        return false;
    }

    app->environment = environment_3d_create();
    if (app->environment == NULL || !environment_3d_init(app->environment, app->width, app->height)) {
        fputs("game_app: failed to initialize environment.\n", stderr);
        environment_3d_destroy(app->environment);
        app->environment = NULL;
        camera_3d_destroy(app->camera);
        app->camera = NULL;
        return false;
    }

    app->mode = GAME_APP_MODE_GAMEPLAY;
    return true;
}

static void stop_gameplay(GameApp *app) {
    if (app == NULL) {
        return;
    }

    environment_3d_destroy(app->environment);
    app->environment = NULL;

    camera_3d_destroy(app->camera);
    app->camera = NULL;

    app->mode = GAME_APP_MODE_MENU;
    start_menu_reset(&app->menu);
}

bool game_app_init(GameApp *app, void *native_display, void *native_window, uint32_t native_window_type, uint16_t width, uint16_t height) {
    if (app == NULL) {
        return false;
    }

    memset(app, 0, sizeof(*app));

    if (!bgfx_renderer_init(&app->renderer, native_display, native_window, native_window_type, width, height)) {
        return false;
    }

    start_menu_init(&app->menu);
    app->width = width;
    app->height = height;
    app->mode = GAME_APP_MODE_MENU;
    return true;
}

void game_app_destroy(GameApp *app) {
    if (app == NULL) {
        return;
    }

    stop_gameplay(app);

    bgfx_renderer_shutdown(&app->renderer);
}

void game_app_resize(GameApp *app, uint16_t width, uint16_t height) {
    if (app == NULL) {
        return;
    }

    app->width = width;
    app->height = height;
    bgfx_renderer_resize(&app->renderer, width, height);

    if (app->environment != NULL) {
        environment_3d_resize(app->environment, width, height);
    }
}

GameAppAction game_app_update(GameApp *app, const GameFrameInput *input, float delta_seconds) {
    if (app == NULL || input == NULL) {
        return GAME_APP_ACTION_QUIT;
    }

    if (input->quit_requested) {
        return GAME_APP_ACTION_QUIT;
    }

    if (app->mode == GAME_APP_MODE_MENU) {
        StartMenuInput menu_input = {
            .move_up = input->menu_up,
            .move_down = input->menu_down,
            .activate = input->menu_activate,
            .mouse_moved = input->mouse_moved,
            .mouse_click = input->mouse_click,
            .mouse_x = input->mouse_x,
            .mouse_y = input->mouse_y,
            .width = app->width,
            .height = app->height,
        };

        const StartMenuAction menu_action = start_menu_update(&app->menu, &menu_input);
        if (menu_action == START_MENU_ACTION_START && start_gameplay(app)) {
            return GAME_APP_ACTION_STARTED_GAMEPLAY;
        }
        if (menu_action == START_MENU_ACTION_QUIT) {
            return GAME_APP_ACTION_QUIT;
        }
        return GAME_APP_ACTION_NONE;
    }

    if (app->mode == GAME_APP_MODE_GAMEPLAY && app->camera != NULL) {
        CameraInputState camera_input = {
            .move_forward = input->move_forward,
            .move_backward = input->move_backward,
            .move_left = input->move_left,
            .move_right = input->move_right,
            .move_up = input->move_up,
            .move_down = input->move_down,
            .mouse_delta_x = input->mouse_delta_x,
            .mouse_delta_y = input->mouse_delta_y,
        };

        camera_3d_update(app->camera, &camera_input, delta_seconds);
    }

    return GAME_APP_ACTION_NONE;
}

void game_app_render(GameApp *app) {
    if (app == NULL) {
        return;
    }

    if (app->mode == GAME_APP_MODE_MENU) {
        bgfx_set_debug(BGFX_DEBUG_TEXT);
        start_menu_render(&app->menu, app->width, app->height);
    } else if (app->environment != NULL) {
        bgfx_set_debug(0);
        environment_3d_frame(app->environment, app->camera);
    }

    bgfx_renderer_frame(&app->renderer);
}

GameAppMode game_app_mode(const GameApp *app) {
    return app != NULL ? app->mode : GAME_APP_MODE_MENU;
}