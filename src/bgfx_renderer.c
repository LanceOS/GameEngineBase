#include "../include/bgfx_renderer.h"

#include <bgfx/c99/bgfx.h>

#include <stdio.h>

static uint16_t clamp_dimension(uint16_t value) {
    return value == 0 ? 1 : value;
}

bool bgfx_renderer_init(BgfxRenderer *renderer, void *native_display, void *native_window, uint16_t width, uint16_t height) {
    if (renderer == NULL || native_window == NULL) {
        return false;
    }

    const uint16_t clamped_width = clamp_dimension(width);
    const uint16_t clamped_height = clamp_dimension(height);

    bgfx_platform_data_t platform_data = {0};
    platform_data.ndt = native_display;
    platform_data.nwh = native_window;
    bgfx_set_platform_data(&platform_data);

    bgfx_init_t init;
    bgfx_init_ctor(&init);
    init.type = BGFX_RENDERER_TYPE_COUNT;
    init.resolution.width = clamped_width;
    init.resolution.height = clamped_height;
    init.resolution.reset = BGFX_RESET_VSYNC;

    if (!bgfx_init(&init)) {
        fputs("bgfx_init failed.\n", stderr);
        renderer->initialized = false;
        return false;
    }

    renderer->width = clamped_width;
    renderer->height = clamped_height;
    renderer->initialized = true;

    bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x1a1f2cff, 1.0f, 0);
    bgfx_set_view_rect(0, 0, 0, renderer->width, renderer->height);
    return true;
}

void bgfx_renderer_resize(BgfxRenderer *renderer, uint16_t width, uint16_t height) {
    if (renderer == NULL || !renderer->initialized) {
        return;
    }

    renderer->width = clamp_dimension(width);
    renderer->height = clamp_dimension(height);

    bgfx_reset(renderer->width, renderer->height, BGFX_RESET_VSYNC, BGFX_TEXTURE_FORMAT_COUNT);
    bgfx_set_view_rect(0, 0, 0, renderer->width, renderer->height);
}

void bgfx_renderer_frame(BgfxRenderer *renderer) {
    if (renderer == NULL || !renderer->initialized) {
        return;
    }

    bgfx_touch(0);
    bgfx_frame(false);
}

void bgfx_renderer_shutdown(BgfxRenderer *renderer) {
    if (renderer == NULL || !renderer->initialized) {
        return;
    }

    bgfx_shutdown();
    renderer->initialized = false;
}