#include "environment/environment_3d.h"

#include <bgfx/c99/bgfx.h>
#include <bx/math.h>

#include <new>
#include <cstdio>
#include <cstring>

namespace {

static constexpr uint16_t kViewId = 0;
static constexpr uint32_t kClearColor = 0x1a1f2cff;

static uint16_t clamp_dimension(uint16_t value) {
    return value == 0 ? 1 : value;
}

class Environment3DImpl {
public:
    Environment3DImpl()
        : width(0)
        , height(0)
        , homogeneous_depth(false)
        , initialized(false) {
        std::memset(view, 0, sizeof(view));
        std::memset(proj, 0, sizeof(proj));
    }

    bool init(uint16_t new_width, uint16_t new_height) {
        const bgfx_caps_t *caps = bgfx_get_caps();
        if (caps == nullptr) {
            std::fputs("environment_3d_init failed: bgfx caps unavailable.\n", stderr);
            return false;
        }

        width = clamp_dimension(new_width);
        height = clamp_dimension(new_height);
        homogeneous_depth = caps->homogeneousDepth;
        initialized = true;

        rebuild_matrices();
        apply_view_state();
        return true;
    }

    void resize(uint16_t new_width, uint16_t new_height) {
        if (!initialized) {
            return;
        }

        width = clamp_dimension(new_width);
        height = clamp_dimension(new_height);
        rebuild_matrices();
        apply_view_state();
    }

    void frame() {
        if (!initialized) {
            return;
        }

        apply_view_state();
        bgfx_touch(kViewId);
    }

private:
    void rebuild_matrices() {
        const bx::Vec3 eye(0.0f, 1.75f, -4.0f);
        const bx::Vec3 at(0.0f, 0.0f, 0.0f);
        const bx::Vec3 up(0.0f, 1.0f, 0.0f);

        bx::mtxLookAt(view, eye, at, up);
        bx::mtxProj(proj, 60.0f, float(width) / float(height), 0.1f, 100.0f, homogeneous_depth);
    }

    void apply_view_state() const {
        bgfx_set_view_clear(kViewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, kClearColor, 1.0f, 0);
        bgfx_set_view_rect(kViewId, 0, 0, width, height);
        bgfx_set_view_transform(kViewId, view, proj);
    }

    uint16_t width;
    uint16_t height;
    bool homogeneous_depth;
    bool initialized;
    float view[16];
    float proj[16];
};

} // namespace

struct Environment3D {
    Environment3DImpl impl;
};

extern "C" {

Environment3D *environment_3d_create(void) {
    return new (std::nothrow) Environment3D();
}

void environment_3d_destroy(Environment3D *environment) {
    delete environment;
}

bool environment_3d_init(Environment3D *environment, uint16_t width, uint16_t height) {
    if (environment == nullptr) {
        return false;
    }

    return environment->impl.init(width, height);
}

void environment_3d_resize(Environment3D *environment, uint16_t width, uint16_t height) {
    if (environment == nullptr) {
        return;
    }

    environment->impl.resize(width, height);
}

void environment_3d_frame(Environment3D *environment) {
    if (environment == nullptr) {
        return;
    }

    environment->impl.frame();
}

} // extern "C"
