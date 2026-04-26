#ifndef START_MENU_H
#define START_MENU_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum StartMenuAction {
    START_MENU_ACTION_NONE = 0,
    START_MENU_ACTION_START,
    START_MENU_ACTION_QUIT,
} StartMenuAction;

typedef struct StartMenuInput {
    bool move_up;
    bool move_down;
    bool activate;
    bool mouse_moved;
    bool mouse_click;
    int mouse_x;
    int mouse_y;
    uint16_t width;
    uint16_t height;
} StartMenuInput;

typedef struct StartMenu {
    int selected_index;
} StartMenu;

void start_menu_init(StartMenu *menu);
void start_menu_reset(StartMenu *menu);
StartMenuAction start_menu_update(StartMenu *menu, const StartMenuInput *input);
void start_menu_render(const StartMenu *menu, uint16_t width, uint16_t height);

#ifdef __cplusplus
}
#endif

#endif