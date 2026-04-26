#include "ui/start_menu.h"

#include <bgfx/c99/bgfx.h>

#include <stddef.h>
#include <string.h>

static uint16_t clamp_text_columns(uint16_t width) {
    uint16_t columns = width / 8;
    return columns == 0 ? 1 : columns;
}

static uint16_t clamp_text_rows(uint16_t height) {
    uint16_t rows = height / 16;
    return rows == 0 ? 1 : rows;
}

static uint16_t center_text_x(uint16_t columns, size_t text_length) {
    if (columns <= text_length) {
        return 0;
    }

    return (uint16_t)((columns - text_length) / 2);
}

static int option_from_point(int mouse_x, int mouse_y, uint16_t width, uint16_t height) {
    const int left = (int)((float)width * 0.35f);
    const int right = (int)((float)width * 0.65f);
    const int start_top = (int)((float)height * 0.42f);
    const int start_bottom = (int)((float)height * 0.52f);
    const int quit_top = (int)((float)height * 0.55f);
    const int quit_bottom = (int)((float)height * 0.65f);

    if (mouse_x >= left && mouse_x <= right) {
        if (mouse_y >= start_top && mouse_y <= start_bottom) {
            return 0;
        }
        if (mouse_y >= quit_top && mouse_y <= quit_bottom) {
            return 1;
        }
    }

    return -1;
}

static StartMenuAction action_for_index(int index) {
    return index == 0 ? START_MENU_ACTION_START : START_MENU_ACTION_QUIT;
}

void start_menu_init(StartMenu *menu) {
    if (menu == NULL) {
        return;
    }

    menu->selected_index = 0;
}

void start_menu_reset(StartMenu *menu) {
    start_menu_init(menu);
}

StartMenuAction start_menu_update(StartMenu *menu, const StartMenuInput *input) {
    if (menu == NULL || input == NULL) {
        return START_MENU_ACTION_NONE;
    }

    const int hovered_index = input->mouse_moved ? option_from_point(input->mouse_x, input->mouse_y, input->width, input->height) : -1;
    if (hovered_index >= 0) {
        menu->selected_index = hovered_index;
    }

    if (input->move_up) {
        menu->selected_index = 0;
    } else if (input->move_down) {
        menu->selected_index = 1;
    }

    if (input->mouse_click) {
        const int clicked_index = option_from_point(input->mouse_x, input->mouse_y, input->width, input->height);
        if (clicked_index >= 0) {
            menu->selected_index = clicked_index;
            return action_for_index(clicked_index);
        }
    }

    if (input->activate) {
        return action_for_index(menu->selected_index);
    }

    return START_MENU_ACTION_NONE;
}

void start_menu_render(const StartMenu *menu, uint16_t width, uint16_t height) {
    const int selected_index = menu != NULL ? menu->selected_index : 0;
    const uint16_t columns = clamp_text_columns(width);
    const uint16_t rows = clamp_text_rows(height);
    const uint16_t title_y = rows > 6 ? (uint16_t)(rows / 2 - 5) : 0;
    const uint16_t start_y = rows > 2 ? (uint16_t)(rows / 2 - 1) : 0;
    const uint16_t quit_y = rows > 0 ? (uint16_t)(rows / 2 + 1) : 1;
    const uint16_t help_y = rows > 0 ? (uint16_t)(rows / 2 + 4) : 1;
    const uint16_t title_x = center_text_x(columns, strlen("Game"));
    const uint16_t option_x = center_text_x(columns, strlen("> Start"));
    const uint16_t help_x = center_text_x(columns, strlen("Enter or click to start"));

    bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x10151fff, 1.0f, 0);
    bgfx_set_view_rect(0, 0, 0, width, height);
    bgfx_touch(0);

    bgfx_set_debug(BGFX_DEBUG_TEXT);
    bgfx_dbg_text_clear(0, false);
    bgfx_dbg_text_printf(title_x, title_y, 0x4f, "Game");
    bgfx_dbg_text_printf(option_x, start_y, selected_index == 0 ? 0x0f : 0x07, "%s Start", selected_index == 0 ? ">" : " ");
    bgfx_dbg_text_printf(option_x, quit_y, selected_index == 1 ? 0x0f : 0x07, "%s Quit", selected_index == 1 ? ">" : " ");
    bgfx_dbg_text_printf(help_x, help_y, 0x2f, "Arrows or mouse to choose");
    bgfx_dbg_text_printf(help_x, (uint16_t)(help_y + 1), 0x2f, "Enter or click to start, Esc to quit");
}