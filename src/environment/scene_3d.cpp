#include "environment/scene_3d.h"

#include <bgfx/c99/bgfx.h>
#include <bx/math.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <new>
#include <string>
#include <system_error>
#include <vector>

namespace {

static constexpr uint16_t kViewId = 0;
static constexpr uint64_t kStateFlags = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS;

struct Vertex {
    float position[3];
    float normal[3];
};

static constexpr std::array<Vertex, 24> kCubeVertices = {{
    {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},
    {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},
    {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},
    {{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},

    {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},
    {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},
    {{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}},
    {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}},
    {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}},

    {{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}},

    {{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}},
    {{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}},

    {{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}},
    {{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}},
}};

static constexpr std::array<uint16_t, 36> kCubeIndices = {{
     0,  1,  2,  0,  2,  3,
     4,  5,  6,  4,  6,  7,
     8,  9, 10,  8, 10, 11,
    12, 13, 14, 12, 14, 15,
    16, 17, 18, 16, 18, 19,
    20, 21, 22, 20, 22, 23,
}};

static bool handle_is_valid(bgfx_program_handle_t handle) {
    return handle.idx != UINT16_MAX;
}

static bool handle_is_valid(bgfx_vertex_buffer_handle_t handle) {
    return handle.idx != UINT16_MAX;
}

static bool handle_is_valid(bgfx_index_buffer_handle_t handle) {
    return handle.idx != UINT16_MAX;
}

static bool handle_is_valid(bgfx_uniform_handle_t handle) {
    return handle.idx != UINT16_MAX;
}

static bool handle_is_valid(bgfx_shader_handle_t handle) {
    return handle.idx != UINT16_MAX;
}

static const char *renderer_backend_directory(bgfx_renderer_type_t type) {
    switch (type) {
        case BGFX_RENDERER_TYPE_DIRECT3D11:
            return "dxbc";
        case BGFX_RENDERER_TYPE_DIRECT3D12:
            return "dxil";
        case BGFX_RENDERER_TYPE_AGC:
        case BGFX_RENDERER_TYPE_GNM:
            return "pssl";
        case BGFX_RENDERER_TYPE_METAL:
            return "metal";
        case BGFX_RENDERER_TYPE_NVN:
            return "nvn";
        case BGFX_RENDERER_TYPE_OPENGL:
            return "glsl";
        case BGFX_RENDERER_TYPE_OPENGLES:
            return "essl";
        case BGFX_RENDERER_TYPE_VULKAN:
            return "spirv";
        case BGFX_RENDERER_TYPE_WEBGPU:
            return "wgsl";
        case BGFX_RENDERER_TYPE_COUNT:
        default:
            return "glsl";
    }
}

static std::filesystem::path find_shader_root() {
    std::error_code error;
    std::filesystem::path probe = std::filesystem::current_path(error);
    if (error) {
        return {};
    }

    for (int depth = 0; depth < 8 && !probe.empty(); ++depth) {
        const std::filesystem::path candidate_roots[] = {
            probe / "build/_deps/bgfx_cmake-src/bgfx/examples/runtime/shaders",
            probe / "_deps/bgfx_cmake-src/bgfx/examples/runtime/shaders",
        };

        for (const std::filesystem::path &candidate : candidate_roots) {
            if (std::filesystem::is_directory(candidate, error) && !error) {
                return candidate;
            }
            error.clear();
        }

        if (!probe.has_parent_path() || probe == probe.parent_path()) {
            break;
        }
        probe = probe.parent_path();
    }

    return {};
}

static std::vector<uint8_t> read_binary_file(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream.is_open()) {
        return {};
    }

    const std::streamsize size = stream.tellg();
    if (size <= 0) {
        return {};
    }

    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    stream.seekg(0, std::ios::beg);
    if (!stream.read(reinterpret_cast<char *>(bytes.data()), size)) {
        return {};
    }

    return bytes;
}

static bgfx_shader_handle_t load_shader(const std::filesystem::path &shader_root, const char *backend, const char *shader_name) {
    const std::filesystem::path shader_path = shader_root / backend / (std::string(shader_name) + ".bin");
    const std::vector<uint8_t> bytes = read_binary_file(shader_path);
    if (bytes.empty()) {
        std::fprintf(stderr, "scene_3d: failed to read shader %s\n", shader_path.string().c_str());
        return BGFX_INVALID_HANDLE;
    }

    const bgfx_memory_t *memory = bgfx_alloc(static_cast<uint32_t>(bytes.size()));
    std::memcpy(memory->data, bytes.data(), bytes.size());

    bgfx_shader_handle_t handle = bgfx_create_shader(memory);
    return handle;
}

static bool encode_cube_mesh(bgfx_vertex_buffer_handle_t *vertex_buffer, bgfx_index_buffer_handle_t *index_buffer) {
    bgfx_vertex_layout_t layout;
    bgfx_vertex_layout_begin(&layout, bgfx_get_renderer_type());
    bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_NORMAL, 3, BGFX_ATTRIB_TYPE_UINT8, true, false);
    bgfx_vertex_layout_end(&layout);

    const uint16_t stride = bgfx_vertex_layout_get_stride(&layout);
    const bgfx_memory_t *vertex_memory = bgfx_alloc(static_cast<uint32_t>(kCubeVertices.size() * stride));
    std::memset(vertex_memory->data, 0, vertex_memory->size);

    for (uint32_t index = 0; index < kCubeVertices.size(); ++index) {
        const Vertex &vertex = kCubeVertices[index];

        float packed_position[4] = {vertex.position[0], vertex.position[1], vertex.position[2], 1.0f};
        float packed_normal[4] = {
            vertex.normal[0] * 0.5f + 0.5f,
            vertex.normal[1] * 0.5f + 0.5f,
            vertex.normal[2] * 0.5f + 0.5f,
            0.0f,
        };

        bgfx_vertex_pack(packed_position, false, BGFX_ATTRIB_POSITION, &layout, vertex_memory->data, index);
        bgfx_vertex_pack(packed_normal, true, BGFX_ATTRIB_NORMAL, &layout, vertex_memory->data, index);
    }

    *vertex_buffer = bgfx_create_vertex_buffer(vertex_memory, &layout, 0);

    const bgfx_memory_t *index_memory = bgfx_alloc(static_cast<uint32_t>(kCubeIndices.size() * sizeof(uint16_t)));
    std::memcpy(index_memory->data, kCubeIndices.data(), index_memory->size);
    *index_buffer = bgfx_create_index_buffer(index_memory, 0);

    return handle_is_valid(*vertex_buffer) && handle_is_valid(*index_buffer);
}

static void pack_transform(float out_mtx[16], float sx, float sy, float sz, float tx, float ty, float tz) {
    bx::mtxSRT(out_mtx, sx, sy, sz, 0.0f, 0.0f, 0.0f, tx, ty, tz);
}

static void submit_instance(bgfx_view_id_t view_id, bgfx_program_handle_t program, bgfx_vertex_buffer_handle_t vertex_buffer, bgfx_index_buffer_handle_t index_buffer, bgfx_uniform_handle_t time_uniform, float time_seconds, const float model[16]) {
    const float uniforms[4] = {time_seconds, 0.0f, 0.0f, 0.0f};

    bgfx_set_transform(model, 1);
    bgfx_set_vertex_buffer(0, vertex_buffer, 0, static_cast<uint32_t>(kCubeVertices.size()));
    bgfx_set_index_buffer(index_buffer, 0, static_cast<uint32_t>(kCubeIndices.size()));
    bgfx_set_state(kStateFlags, 0);
    bgfx_set_uniform(time_uniform, uniforms, 1);
    bgfx_submit(view_id, program, 0, 0);
}

} // namespace

struct Scene3D {
    bgfx_program_handle_t program;
    bgfx_vertex_buffer_handle_t vertex_buffer;
    bgfx_index_buffer_handle_t index_buffer;
    bgfx_uniform_handle_t time_uniform;
    bool initialized;
    float time_seconds;

    Scene3D()
        : program(BGFX_INVALID_HANDLE)
        , vertex_buffer(BGFX_INVALID_HANDLE)
        , index_buffer(BGFX_INVALID_HANDLE)
        , time_uniform(BGFX_INVALID_HANDLE)
        , initialized(false)
        , time_seconds(0.0f) {
    }

    ~Scene3D() {
        shutdown();
    }

    bool init() {
        shutdown();

        const std::filesystem::path shader_root = find_shader_root();
        if (shader_root.empty()) {
            std::fputs("scene_3d: shader runtime root not found.\n", stderr);
            return false;
        }

        const char *backend = renderer_backend_directory(bgfx_get_renderer_type());
        bgfx_shader_handle_t vertex_shader = load_shader(shader_root, backend, "vs_mesh");
        bgfx_shader_handle_t fragment_shader = load_shader(shader_root, backend, "fs_mesh");

        if (!handle_is_valid(vertex_shader) || !handle_is_valid(fragment_shader)) {
            if (handle_is_valid(vertex_shader)) {
                bgfx_destroy_shader(vertex_shader);
            }
            if (handle_is_valid(fragment_shader)) {
                bgfx_destroy_shader(fragment_shader);
            }
            std::fprintf(stderr, "scene_3d: failed to load shader pair from %s/%s.\n", shader_root.string().c_str(), backend);
            return false;
        }

        program = bgfx_create_program(vertex_shader, fragment_shader, true);
        if (!handle_is_valid(program)) {
            std::fputs("scene_3d: bgfx_create_program failed.\n", stderr);
            shutdown();
            return false;
        }

        time_uniform = bgfx_create_uniform("u_time", BGFX_UNIFORM_TYPE_VEC4, 1);
        if (!handle_is_valid(time_uniform)) {
            std::fputs("scene_3d: bgfx_create_uniform failed for u_time.\n", stderr);
            shutdown();
            return false;
        }

        if (!encode_cube_mesh(&vertex_buffer, &index_buffer)) {
            std::fputs("scene_3d: failed to build cube mesh buffers.\n", stderr);
            shutdown();
            return false;
        }

        initialized = true;
        return true;
    }

    void frame() {
        if (!initialized) {
            return;
        }

        time_seconds += 1.0f / 60.0f;

        float floor_mtx[16];
        float object_mtx[16];
        pack_transform(floor_mtx, 12.0f, 0.1f, 12.0f, 0.0f, -0.05f, 0.0f);
        pack_transform(object_mtx, 0.75f, 1.5f, 0.75f, 0.0f, 0.75f, 0.0f);

        submit_instance(kViewId, program, vertex_buffer, index_buffer, time_uniform, time_seconds, floor_mtx);
        submit_instance(kViewId, program, vertex_buffer, index_buffer, time_uniform, time_seconds, object_mtx);
    }

    void shutdown() {
        if (handle_is_valid(program)) {
            bgfx_destroy_program(program);
            program = BGFX_INVALID_HANDLE;
        }

        if (handle_is_valid(vertex_buffer)) {
            bgfx_destroy_vertex_buffer(vertex_buffer);
            vertex_buffer = BGFX_INVALID_HANDLE;
        }

        if (handle_is_valid(index_buffer)) {
            bgfx_destroy_index_buffer(index_buffer);
            index_buffer = BGFX_INVALID_HANDLE;
        }

        if (handle_is_valid(time_uniform)) {
            bgfx_destroy_uniform(time_uniform);
            time_uniform = BGFX_INVALID_HANDLE;
        }

        initialized = false;
        time_seconds = 0.0f;
    }
};

extern "C" {

Scene3D *scene_3d_create(void) {
    return new (std::nothrow) Scene3D();
}

void scene_3d_destroy(Scene3D *scene) {
    delete scene;
}

bool scene_3d_init(Scene3D *scene) {
    if (scene == nullptr) {
        return false;
    }

    return scene->init();
}

void scene_3d_frame(Scene3D *scene) {
    if (scene == nullptr) {
        return;
    }

    scene->frame();
}

} // extern "C"
