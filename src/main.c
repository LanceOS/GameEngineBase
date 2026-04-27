#define _POSIX_C_SOURCE 199309L

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "app/game_app.h"
#include "controls/controls.h"

typedef struct GameState {
    Display *display;
    int screen;
    Window window;
    Atom wm_delete_window;
    bool running;
    bool resized;
    uint16_t width;
    uint16_t height;
    int mouse_x;
    int mouse_y;
    int previous_mouse_x;
    int previous_mouse_y;
    bool mouse_click;
    bool menu_up;
    bool menu_down;
    bool menu_activate;
    bool move_forward;
    bool move_backward;
    bool move_left;
    bool move_right;
    bool move_up;
    bool move_down;
    bool quit_requested;
} GameState;

static void sleep_for_frame(void) {
    const struct timespec frame_delay = {
        .tv_sec = 0,
        .tv_nsec = 16L * 1000L * 1000L,
    };

    nanosleep(&frame_delay, NULL);
}

static bool query_window_origin(GameState *game, int *origin_x, int *origin_y) {
    if (game == NULL || game->display == NULL || origin_x == NULL || origin_y == NULL) {
        return false;
    }

    Window root_window = RootWindow(game->display, game->screen);
    Window child_window = None;
    int translated_x = 0;
    int translated_y = 0;

    if (XTranslateCoordinates(game->display, game->window, root_window, 0, 0, &translated_x, &translated_y, &child_window) == 0) {
        return false;
    }

    *origin_x = translated_x;
    *origin_y = translated_y;
    return true;
}

static bool query_pointer_root_position(GameState *game, int *root_x, int *root_y) {
    if (game == NULL || game->display == NULL || root_x == NULL || root_y == NULL) {
        return false;
    }

    Window root_return = None;
    Window child_return = None;
    int window_x = 0;
    int window_y = 0;
    unsigned int mask_return = 0;

    if (XQueryPointer(game->display, game->window, &root_return, &child_return, root_x, root_y, &window_x, &window_y, &mask_return) == False) {
        return false;
    }

    return true;
}

static bool create_window(GameState *game) {
    game->display = XOpenDisplay(NULL);
    if (game->display == NULL) {
        fputs("Failed to open X display.\n", stderr);
        return false;
    }

    game->screen = DefaultScreen(game->display);

    const Window root_window = RootWindow(game->display, game->screen);
    const unsigned long border_color = BlackPixel(game->display, game->screen);
    const unsigned long background_color = WhitePixel(game->display, game->screen);

    game->window = XCreateSimpleWindow(
        game->display,
        root_window,
        100,
        100,
        800,
        600,
        1,
        border_color,
        background_color);

    if (game->window == 0) {
        fputs("Failed to create X window.\n", stderr);
        XCloseDisplay(game->display);
        game->display = NULL;
        return false;
    }

    XStoreName(game->display, game->window, "Game");

    const long event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;
    XSelectInput(game->display, game->window, event_mask);

    game->wm_delete_window = XInternAtom(game->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(game->display, game->window, &game->wm_delete_window, 1);

    XMapWindow(game->display, game->window);
    XFlush(game->display);

    return true;
}

static void destroy_window(GameState *game) {
    if (game->display != NULL) {
        if (game->window != 0) {
            XDestroyWindow(game->display, game->window);
            game->window = 0;
        }

        XCloseDisplay(game->display);
        game->display = NULL;
    }
}

static void process_event(GameState *game, XEvent *event) {
    switch (event->type) {
        case ClientMessage:
            if ((Atom)event->xclient.data.l[0] == game->wm_delete_window) {
                game->quit_requested = true;
            }
            break;

        case DestroyNotify:
            game->quit_requested = true;
            break;

        case ConfigureNotify:
            game->width = (uint16_t)event->xconfigure.width;
            game->height = (uint16_t)event->xconfigure.height;
            game->resized = true;
            break;

        case MotionNotify:
            game->mouse_x = event->xmotion.x_root;
            game->mouse_y = event->xmotion.y_root;
            break;

        case ButtonPress:
            if (event->xbutton.button == Button1) {
                game->mouse_click = true;
            }
            game->mouse_x = event->xbutton.x_root;
            game->mouse_y = event->xbutton.y_root;
            break;

        case KeyPress: {
            const KeySym symbol = XLookupKeysym(&event->xkey, 0);
            switch (symbol) {
                case XK_Escape:
                    game->quit_requested = true;
                    break;

                case XK_Up:
                    game->menu_up = true;
                    break;

                case XK_Down:
                    game->menu_down = true;
                    break;

                case XK_Return:
                case XK_KP_Enter:
                case XK_space:
                    game->menu_activate = true;
                    game->move_up = true;
                    break;

                case XK_w:
                case XK_W:
                    game->move_forward = true;
                    break;

                case XK_s:
                case XK_S:
                    game->move_backward = true;
                    break;

                case XK_a:
                case XK_A:
                    game->move_left = true;
                    break;

                case XK_d:
                case XK_D:
                    game->move_right = true;
                    break;

                case XK_Control_L:
                case XK_Control_R:
                    game->move_down = true;
                    break;

                default:
                    break;
            }
            break;
        }

        case KeyRelease: {
            const KeySym symbol = XLookupKeysym(&event->xkey, 0);
            switch (symbol) {
                case XK_w:
                case XK_W:
                    game->move_forward = false;
                    break;

                case XK_s:
                case XK_S:
                    game->move_backward = false;
                    break;

                case XK_a:
                case XK_A:
                    game->move_left = false;
                    break;

                case XK_d:
                case XK_D:
                    game->move_right = false;
                    break;

                case XK_space:
                    game->move_up = false;
                    break;

                case XK_Control_L:
                case XK_Control_R:
                    game->move_down = false;
                    break;

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }
}

int main(void) {
    GameState game = {
        .display = NULL,
        .screen = 0,
        .window = 0,
        .wm_delete_window = None,
        .running = true,
        .resized = false,
        .width = 800,
        .height = 600,
        .mouse_x = 0,
        .mouse_y = 0,
        .previous_mouse_x = 0,
        .previous_mouse_y = 0,
        .mouse_click = false,
        .menu_up = false,
        .menu_down = false,
        .menu_activate = false,
        .move_forward = false,
        .move_backward = false,
        .move_left = false,
        .move_right = false,
        .move_up = false,
        .move_down = false,
        .quit_requested = false,
    };

    Controls *controls = NULL;

    if (!create_window(&game)) {
        return EXIT_FAILURE;
    }

    controls = controls_create_for_x11(game.display, (unsigned long)game.window, game.screen);
    if (controls == NULL) {
        destroy_window(&game);
        return EXIT_FAILURE;
    }

    GameApp app;
    if (!game_app_init(&app, game.display, (void *)(uintptr_t)game.window, 0, game.width, game.height)) {
        destroy_window(&game);
        return EXIT_FAILURE;
    }

    while (game.running) {
        controls_poll_events(controls);

        uint16_t resized_w = 0, resized_h = 0;
        if (controls_consume_resize(controls, &resized_w, &resized_h)) {
            game_app_resize(&app, resized_w, resized_h);
        }

        GameFrameInput input = {0};
        controls_get_frame_input(controls, &input, false);

        GameAppAction action = game_app_update(&app, &input, 1.0f / 60.0f);
        if (action == GAME_APP_ACTION_QUIT) {
            game.running = false;
        } else {
            game_app_render(&app);
        }

        sleep_for_frame();
    }

    game_app_destroy(&app);
    controls_destroy(controls);
    destroy_window(&game);
    return EXIT_SUCCESS;
}
