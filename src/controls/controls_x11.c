#define _POSIX_C_SOURCE 199309L

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "controls/controls.h"

struct Controls {
    Display *display;
    int screen;
    Window window;
    Atom wm_delete_window;
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
};

static bool query_window_origin(Controls *c, int *origin_x, int *origin_y) {
    if (c == NULL || c->display == NULL || origin_x == NULL || origin_y == NULL) {
        return false;
    }

    Window root_window = RootWindow(c->display, c->screen);
    Window child_window = None;
    int translated_x = 0;
    int translated_y = 0;

    if (XTranslateCoordinates(c->display, c->window, root_window, 0, 0, &translated_x, &translated_y, &child_window) == 0) {
        return false;
    }

    *origin_x = translated_x;
    *origin_y = translated_y;
    return true;
}

static bool query_pointer_root_position(Controls *c, int *root_x, int *root_y) {
    if (c == NULL || c->display == NULL || root_x == NULL || root_y == NULL) {
        return false;
    }

    Window root_return = None;
    Window child_return = None;
    int window_x = 0;
    int window_y = 0;
    unsigned int mask_return = 0;

    if (XQueryPointer(c->display, c->window, &root_return, &child_return, root_x, root_y, &window_x, &window_y, &mask_return) == False) {
        return false;
    }

    return true;
}

Controls *controls_create_for_x11(void *display, unsigned long window, int screen) {
    if (display == NULL) return NULL;
    Controls *c = (Controls *)malloc(sizeof(Controls));
    if (c == NULL) return NULL;
    memset(c, 0, sizeof(*c));
    c->display = (Display *)display;
    c->screen = screen;
    c->window = (Window)window;
    c->wm_delete_window = XInternAtom(c->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(c->display, c->window, &c->wm_delete_window, 1);
    return c;
}

void controls_destroy(Controls *c) {
    if (c == NULL) return;
    free(c);
}

static void process_event(Controls *c, XEvent *event) {
    if (c == NULL || event == NULL) return;
    switch (event->type) {
        case ClientMessage:
            if ((Atom)event->xclient.data.l[0] == c->wm_delete_window) {
                c->quit_requested = true;
            }
            break;

        case DestroyNotify:
            c->quit_requested = true;
            break;

        case ConfigureNotify:
            c->width = (uint16_t)event->xconfigure.width;
            c->height = (uint16_t)event->xconfigure.height;
            c->resized = true;
            break;

        case MotionNotify:
            c->mouse_x = event->xmotion.x_root;
            c->mouse_y = event->xmotion.y_root;
            break;

        case ButtonPress:
            if (event->xbutton.button == Button1) {
                c->mouse_click = true;
            }
            c->mouse_x = event->xbutton.x_root;
            c->mouse_y = event->xbutton.y_root;
            break;

        case KeyPress: {
            const KeySym symbol = XLookupKeysym(&event->xkey, 0);
            switch (symbol) {
                case XK_Escape:
                    c->quit_requested = true;
                    break;

                case XK_Up:
                    c->menu_up = true;
                    break;

                case XK_Down:
                    c->menu_down = true;
                    break;

                case XK_Return:
                case XK_KP_Enter:
                case XK_space:
                    c->menu_activate = true;
                    c->move_up = true;
                    break;

                case XK_w:
                case XK_W:
                    c->move_forward = true;
                    break;

                case XK_s:
                case XK_S:
                    c->move_backward = true;
                    break;

                case XK_a:
                case XK_A:
                    c->move_left = true;
                    break;

                case XK_d:
                case XK_D:
                    c->move_right = true;
                    break;

                case XK_Control_L:
                case XK_Control_R:
                    c->move_down = true;
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
                    c->move_forward = false;
                    break;

                case XK_s:
                case XK_S:
                    c->move_backward = false;
                    break;

                case XK_a:
                case XK_A:
                    c->move_left = false;
                    break;

                case XK_d:
                case XK_D:
                    c->move_right = false;
                    break;

                case XK_space:
                    c->move_up = false;
                    break;

                case XK_Control_L:
                case XK_Control_R:
                    c->move_down = false;
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

void controls_poll_events(Controls *c) {
    if (c == NULL || c->display == NULL) return;
    while (XPending(c->display) > 0) {
        XEvent event;
        XNextEvent(c->display, &event);
        process_event(c, &event);
    }
}

void controls_get_frame_input(Controls *c, GameFrameInput *out, bool gameplay_mode) {
    if (c == NULL || out == NULL) return;
    memset(out, 0, sizeof(*out));

    if (c->resized) {
        /* leave resize handling to caller who will call game_app_resize */
    }

    int window_origin_x = 0;
    int window_origin_y = 0;
    if (!query_window_origin(c, &window_origin_x, &window_origin_y)) {
        window_origin_x = 0;
        window_origin_y = 0;
    }

    int mouse_root_x = c->mouse_x;
    int mouse_root_y = c->mouse_y;
    if (query_pointer_root_position(c, &mouse_root_x, &mouse_root_y)) {
        c->mouse_x = mouse_root_x;
        c->mouse_y = mouse_root_y;
    }

    const int local_mouse_x = mouse_root_x - window_origin_x;
    const int local_mouse_y = mouse_root_y - window_origin_y;

    out->quit_requested = c->quit_requested;
    out->menu_up = c->menu_up;
    out->menu_down = c->menu_down;
    out->menu_activate = c->menu_activate;
    out->mouse_moved = local_mouse_x != c->previous_mouse_x || local_mouse_y != c->previous_mouse_y;
    out->mouse_click = c->mouse_click;
    out->mouse_x = local_mouse_x;
    out->mouse_y = local_mouse_y;
    out->mouse_delta_x = (float)(local_mouse_x - c->previous_mouse_x);
    out->mouse_delta_y = (float)(local_mouse_y - c->previous_mouse_y);
    out->move_forward = c->move_forward;
    out->move_backward = c->move_backward;
    out->move_left = c->move_left;
    out->move_right = c->move_right;
    out->move_up = c->move_up;
    out->move_down = c->move_down;

    c->previous_mouse_x = local_mouse_x;
    c->previous_mouse_y = local_mouse_y;
    c->menu_up = false;
    c->menu_down = false;
    c->menu_activate = false;
    c->mouse_click = false;
    c->quit_requested = false;
}

bool controls_set_relative_mouse_mode(Controls *c, bool enable) {
    if (c == NULL || c->display == NULL) return false;
    if (enable) {
        int grab = XGrabPointer(c->display, c->window, True, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, c->window, None, CurrentTime);
        XFlush(c->display);
        return grab == GrabSuccess;
    } else {
        XUngrabPointer(c->display, CurrentTime);
        XFlush(c->display);
        return true;
    }
}

void controls_set_cursor_visible(Controls *c, bool visible) {
    (void)c;
    (void)visible;
    /* Cursor visibility is a no-op here; applications can implement custom cursors if needed. */
}

bool controls_consume_resize(Controls *c, uint16_t *width, uint16_t *height) {
    if (c == NULL || width == NULL || height == NULL) return false;
    if (!c->resized) return false;
    *width = c->width;
    *height = c->height;
    c->resized = false;
    return true;
}
