#include "bgfx_renderer.h"

#include <bgfx/c99/bgfx.h>

#include <stdio.h>
#include <string.h>

static uint16_t clamp_dimension(uint16_t value) {
    return value == 0 ? 1 : value;
}

bool bgfx_renderer_init(BgfxRenderer *renderer, void *native_display, void *native_window, uint32_t native_window_type, uint16_t width, uint16_t height) {
    if (renderer == NULL || native_window == NULL) {
        return false;
    }

    const uint16_t clamped_width = clamp_dimension(width);
    const uint16_t clamped_height = clamp_dimension(height);

    /* Try multiple renderer types; print attempts for diagnostics. */
    bgfx_renderer_type_t candidates[] = {
        BGFX_RENDERER_TYPE_COUNT,
        BGFX_RENDERER_TYPE_OPENGL,
        BGFX_RENDERER_TYPE_VULKAN,
#if defined(BGFX_RENDERER_TYPE_METAL)
        BGFX_RENDERER_TYPE_METAL,
#endif
#if defined(BGFX_RENDERER_TYPE_DIRECT3D11)
        BGFX_RENDERER_TYPE_DIRECT3D11,
#endif
    };

    const size_t num_candidates = sizeof(candidates) / sizeof(candidates[0]);
    bool init_ok = false;
    bgfx_init_t init;

    for (size_t i = 0; i < num_candidates; ++i) {
        bgfx_init_ctor(&init);
        init.type = candidates[i];
        init.platformData.ndt = native_display;
        init.platformData.nwh = native_window;
        init.platformData.type = (bgfx_native_window_handle_type_t)native_window_type;
        init.resolution.width = clamped_width;
        init.resolution.height = clamped_height;
        init.resolution.reset = BGFX_RESET_VSYNC;

        fprintf(stderr, "bgfx_init: attempting renderer type %d\n", (int)init.type);
        if (bgfx_init(&init)) {
            fprintf(stderr, "bgfx_init: succeeded with renderer type %d\n", (int)init.type);
            init_ok = true;
            break;
        }
        fprintf(stderr, "bgfx_init: failed for renderer type %d\n", (int)init.type);
    }

    if (!init_ok) {
        fputs("bgfx_init failed (all renderer attempts).\n", stderr);
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