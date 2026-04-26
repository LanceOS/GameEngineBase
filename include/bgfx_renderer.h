#ifndef BGFX_RENDERER_H
#define BGFX_RENDERER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct BgfxRenderer {
    uint16_t width;
    uint16_t height;
    bool initialized;
} BgfxRenderer;

bool bgfx_renderer_init(BgfxRenderer *renderer, void *native_display, void *native_window, uint16_t width, uint16_t height);
void bgfx_renderer_resize(BgfxRenderer *renderer, uint16_t width, uint16_t height);
void bgfx_renderer_frame(BgfxRenderer *renderer);
void bgfx_renderer_shutdown(BgfxRenderer *renderer);

#endif