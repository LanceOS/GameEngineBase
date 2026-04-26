#ifndef GAME_APP_H
#define GAME_APP_H

#include <stdbool.h>
#include <stdint.h>

#include "renderer/bgfx_renderer.h"
#include "ui/start_menu.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Camera3D Camera3D;
typedef struct Environment3D Environment3D;

typedef struct GameFrameInput {
    bool quit_requested;
    bool menu_up;
    bool menu_down;
    bool menu_activate;
    bool mouse_moved;
    bool mouse_click;
    int mouse_x;
    int mouse_y;
    float mouse_delta_x;
    float mouse_delta_y;
    bool move_forward;
    bool move_backward;
    bool move_left;
    bool move_right;
    bool move_up;
    bool move_down;
} GameFrameInput;

typedef enum GameAppAction {
    GAME_APP_ACTION_NONE = 0,
    GAME_APP_ACTION_STARTED_GAMEPLAY,
    GAME_APP_ACTION_QUIT,
} GameAppAction;

typedef enum GameAppMode {
    GAME_APP_MODE_MENU = 0,
    GAME_APP_MODE_GAMEPLAY,
} GameAppMode;

typedef struct GameApp {
    BgfxRenderer renderer;
    StartMenu menu;
    Camera3D *camera;
    Environment3D *environment;
    uint16_t width;
    uint16_t height;
    GameAppMode mode;
} GameApp;

bool game_app_init(GameApp *app, void *native_display, void *native_window, uint32_t native_window_type, uint16_t width, uint16_t height);
void game_app_destroy(GameApp *app);
void game_app_resize(GameApp *app, uint16_t width, uint16_t height);
GameAppAction game_app_update(GameApp *app, const GameFrameInput *input, float delta_seconds);
void game_app_render(GameApp *app);
GameAppMode game_app_mode(const GameApp *app);

#ifdef __cplusplus
}
#endif

#endif