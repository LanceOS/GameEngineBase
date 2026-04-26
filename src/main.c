#define _POSIX_C_SOURCE 199309L

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct GameState {
    Display *display;
    int screen;
    Window window;
    Atom wm_delete_window;
    bool running;
} GameState;

static void sleep_for_frame(void) {
    const struct timespec frame_delay = {
        .tv_sec = 0,
        .tv_nsec = 16L * 1000L * 1000L,
    };

    nanosleep(&frame_delay, NULL);
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

    const long event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;
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
                game->running = false;
            }
            break;

        case DestroyNotify:
            game->running = false;
            break;

        case KeyPress: {
            const KeySym key_symbol = XLookupKeysym(&event->xkey, 0);
            if (key_symbol == XK_Escape) {
                game->running = false;
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
    };

    if (!create_window(&game)) {
        return EXIT_FAILURE;
    }

    while (game.running) {
        while (XPending(game.display) > 0) {
            XEvent event;
            XNextEvent(game.display, &event);
            process_event(&game, &event);
        }

        sleep_for_frame();
    }

    destroy_window(&game);
    return EXIT_SUCCESS;
}
