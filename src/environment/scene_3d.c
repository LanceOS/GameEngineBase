#include "environment/scene_3d.h"

#include <bgfx/c99/bgfx.h>

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#include <io.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static const uint16_t kViewId = 0;
static const uint64_t kStateFlags = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS;

typedef struct Vertex {
    float x;
    float y;
    float z;
    uint32_t abgr;
} Vertex;

static const Vertex kCubeVertices[] = {
    {-1.0f, 1.0f, 1.0f, 0xffffffffu},
    {1.0f, 1.0f, 1.0f, 0xffffffffu},
    {-1.0f, -1.0f, 1.0f, 0xffffffffu},
    {1.0f, -1.0f, 1.0f, 0xffffffffu},
    {-1.0f, 1.0f, -1.0f, 0xffffffffu},
    {1.0f, 1.0f, -1.0f, 0xffffffffu},
    {-1.0f, -1.0f, -1.0f, 0xffffffffu},
    {1.0f, -1.0f, -1.0f, 0xffffffffu},
};

static const uint16_t kCubeIndices[] = {
    0, 1, 2, 1, 3, 2,
    4, 6, 5, 5, 6, 7,
    0, 2, 4, 4, 2, 6,
    1, 5, 3, 5, 7, 3,
    0, 4, 1, 4, 5, 1,
    2, 3, 6, 6, 3, 7,
};

struct Scene3D {
    bgfx_program_handle_t program;
    bgfx_vertex_buffer_handle_t vertex_buffer;
    bgfx_index_buffer_handle_t index_buffer;
    bool initialized;
    float model[16];
};

static bgfx_program_handle_t invalid_program_handle(void) {
    bgfx_program_handle_t handle = {UINT16_MAX};
    return handle;
}

static bgfx_vertex_buffer_handle_t invalid_vertex_buffer_handle(void) {
    bgfx_vertex_buffer_handle_t handle = {UINT16_MAX};
    return handle;
}

static bgfx_index_buffer_handle_t invalid_index_buffer_handle(void) {
    bgfx_index_buffer_handle_t handle = {UINT16_MAX};
    return handle;
}

static bgfx_shader_handle_t invalid_shader_handle(void) {
    bgfx_shader_handle_t handle = {UINT16_MAX};
    return handle;
}

static bool program_handle_valid(bgfx_program_handle_t handle) {
    return handle.idx != UINT16_MAX;
}

static bool vertex_buffer_handle_valid(bgfx_vertex_buffer_handle_t handle) {
    return handle.idx != UINT16_MAX;
}

static bool index_buffer_handle_valid(bgfx_index_buffer_handle_t handle) {
    return handle.idx != UINT16_MAX;
}

static bool shader_handle_valid(bgfx_shader_handle_t handle) {
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

static bool directory_exists(const char *path) {
#if defined(_WIN32) || defined(_WIN64)
    struct _stat info;
    if (_stat(path, &info) != 0) {
        return false;
    }
    return (info.st_mode & _S_IFDIR) != 0;
#else
    struct stat info;
    return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
#endif
}

static bool format_path(char *out, size_t out_size, const char *prefix, const char *suffix) {
    int written;
    if (out == NULL || out_size == 0 || prefix == NULL || suffix == NULL) {
        return false;
    }

    written = snprintf(out, out_size, "%s/%s", prefix, suffix);
    return written > 0 && (size_t)written < out_size;
}

static bool find_shader_root(char *out_path, size_t out_size) {
    char probe[PATH_MAX];
    int depth;

    if (out_path == NULL || out_size == 0) {
        return false;
    }

    if (getcwd(probe, sizeof(probe)) == NULL) {
        return false;
    }

    for (depth = 0; depth < 8; ++depth) {
        char candidate[PATH_MAX];
        char *slash;

        if (format_path(candidate, sizeof(candidate), probe, "build/_deps/bgfx_cmake-src/bgfx/examples/runtime/shaders") && directory_exists(candidate)) {
            if ((strlen(candidate) + 1) <= out_size) {
                strcpy(out_path, candidate);
                return true;
            }
            return false;
        }

        if (format_path(candidate, sizeof(candidate), probe, "_deps/bgfx_cmake-src/bgfx/examples/runtime/shaders") && directory_exists(candidate)) {
            if ((strlen(candidate) + 1) <= out_size) {
                strcpy(out_path, candidate);
                return true;
            }
            return false;
        }

        slash = strrchr(probe, '/');
        if (slash == NULL) {
            break;
        }

        if (slash == probe) {
            probe[1] = '\0';
        } else {
            *slash = '\0';
        }
    }

    return false;
}

static bool read_binary_file(const char *path, uint8_t **out_data, size_t *out_size) {
    FILE *stream;
    long file_size_long;
    size_t read_size;
    uint8_t *bytes;

    if (path == NULL || out_data == NULL || out_size == NULL) {
        return false;
    }

    *out_data = NULL;
    *out_size = 0;

    stream = fopen(path, "rb");
    if (stream == NULL) {
        return false;
    }

    if (fseek(stream, 0, SEEK_END) != 0) {
        fclose(stream);
        return false;
    }

    file_size_long = ftell(stream);
    if (file_size_long <= 0) {
        fclose(stream);
        return false;
    }

    if (fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        return false;
    }

    bytes = (uint8_t *)malloc((size_t)file_size_long);
    if (bytes == NULL) {
        fclose(stream);
        return false;
    }

    read_size = fread(bytes, 1, (size_t)file_size_long, stream);
    fclose(stream);

    if (read_size != (size_t)file_size_long) {
        free(bytes);
        return false;
    }

    *out_data = bytes;
    *out_size = read_size;
    return true;
}

static bgfx_shader_handle_t load_shader(const char *shader_root, const char *backend, const char *shader_name) {
    char shader_path[PATH_MAX];
    uint8_t *bytes;
    size_t size;
    const bgfx_memory_t *memory;
    bgfx_shader_handle_t handle;
    int written;

    written = snprintf(shader_path, sizeof(shader_path), "%s/%s/%s.bin", shader_root, backend, shader_name);
    if (written <= 0 || (size_t)written >= sizeof(shader_path)) {
        return invalid_shader_handle();
    }

    if (!read_binary_file(shader_path, &bytes, &size)) {
        fprintf(stderr, "scene_3d: failed to read shader %s\n", shader_path);
        return invalid_shader_handle();
    }

    if (size > UINT32_MAX) {
        free(bytes);
        fprintf(stderr, "scene_3d: shader too large %s\n", shader_path);
        return invalid_shader_handle();
    }

    memory = bgfx_copy(bytes, (uint32_t)size);
    free(bytes);

    if (memory == NULL) {
        return invalid_shader_handle();
    }

    handle = bgfx_create_shader(memory);
    return handle;
}

static bool encode_cube_mesh(bgfx_vertex_buffer_handle_t *vertex_buffer, bgfx_index_buffer_handle_t *index_buffer) {
    bgfx_vertex_layout_t layout;
    const bgfx_memory_t *vertex_memory;
    const bgfx_memory_t *index_memory;

    if (vertex_buffer == NULL || index_buffer == NULL) {
        return false;
    }

    *vertex_buffer = invalid_vertex_buffer_handle();
    *index_buffer = invalid_index_buffer_handle();

    bgfx_vertex_layout_begin(&layout, bgfx_get_renderer_type());
    bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_COLOR0, 4, BGFX_ATTRIB_TYPE_UINT8, true, false);
    bgfx_vertex_layout_end(&layout);

    vertex_memory = bgfx_copy(kCubeVertices, (uint32_t)sizeof(kCubeVertices));
    if (vertex_memory == NULL) {
        return false;
    }

    *vertex_buffer = bgfx_create_vertex_buffer(vertex_memory, &layout, 0);
    if (!vertex_buffer_handle_valid(*vertex_buffer)) {
        *vertex_buffer = invalid_vertex_buffer_handle();
        return false;
    }

    index_memory = bgfx_copy(kCubeIndices, (uint32_t)sizeof(kCubeIndices));
    if (index_memory == NULL) {
        bgfx_destroy_vertex_buffer(*vertex_buffer);
        *vertex_buffer = invalid_vertex_buffer_handle();
        return false;
    }

    *index_buffer = bgfx_create_index_buffer(index_memory, 0);
    if (!index_buffer_handle_valid(*index_buffer)) {
        bgfx_destroy_vertex_buffer(*vertex_buffer);
        *vertex_buffer = invalid_vertex_buffer_handle();
        *index_buffer = invalid_index_buffer_handle();
        return false;
    }

    return true;
}

static void set_identity(float matrix[16]) {
    size_t i;
    for (i = 0; i < 16; ++i) {
        matrix[i] = 0.0f;
    }

    matrix[0] = 1.0f;
    matrix[5] = 1.0f;
    matrix[10] = 1.0f;
    matrix[15] = 1.0f;
}

static void submit_block(bgfx_program_handle_t program, bgfx_vertex_buffer_handle_t vertex_buffer, bgfx_index_buffer_handle_t index_buffer, const float model[16]) {
    bgfx_set_transform(model, 1);
    bgfx_set_vertex_buffer(0, vertex_buffer, 0, (uint32_t)(sizeof(kCubeVertices) / sizeof(kCubeVertices[0])));
    bgfx_set_index_buffer(index_buffer, 0, (uint32_t)(sizeof(kCubeIndices) / sizeof(kCubeIndices[0])));
    bgfx_set_state(kStateFlags, 0);
    bgfx_submit(kViewId, program, 0, 0);
}

static void scene_3d_shutdown(Scene3D *scene) {
    if (scene == NULL) {
        return;
    }

    if (program_handle_valid(scene->program)) {
        bgfx_destroy_program(scene->program);
        scene->program = invalid_program_handle();
    }

    if (vertex_buffer_handle_valid(scene->vertex_buffer)) {
        bgfx_destroy_vertex_buffer(scene->vertex_buffer);
        scene->vertex_buffer = invalid_vertex_buffer_handle();
    }

    if (index_buffer_handle_valid(scene->index_buffer)) {
        bgfx_destroy_index_buffer(scene->index_buffer);
        scene->index_buffer = invalid_index_buffer_handle();
    }

    scene->initialized = false;
}

Scene3D *scene_3d_create(void) {
    Scene3D *scene = (Scene3D *)calloc(1, sizeof(*scene));
    if (scene == NULL) {
        return NULL;
    }

    scene->program = invalid_program_handle();
    scene->vertex_buffer = invalid_vertex_buffer_handle();
    scene->index_buffer = invalid_index_buffer_handle();
    scene->initialized = false;
    set_identity(scene->model);
    return scene;
}

void scene_3d_destroy(Scene3D *scene) {
    if (scene == NULL) {
        return;
    }

    scene_3d_shutdown(scene);
    free(scene);
}

bool scene_3d_init(Scene3D *scene) {
    char shader_root[PATH_MAX];
    const char *backend;
    bgfx_shader_handle_t vertex_shader;
    bgfx_shader_handle_t fragment_shader;

    if (scene == NULL) {
        return false;
    }

    scene_3d_shutdown(scene);

    if (!find_shader_root(shader_root, sizeof(shader_root))) {
        fputs("scene_3d: shader runtime root not found.\n", stderr);
        return false;
    }

    backend = renderer_backend_directory(bgfx_get_renderer_type());
    vertex_shader = load_shader(shader_root, backend, "vs_cubes");
    fragment_shader = load_shader(shader_root, backend, "fs_cubes");

    if (!shader_handle_valid(vertex_shader) || !shader_handle_valid(fragment_shader)) {
        if (shader_handle_valid(vertex_shader)) {
            bgfx_destroy_shader(vertex_shader);
        }
        if (shader_handle_valid(fragment_shader)) {
            bgfx_destroy_shader(fragment_shader);
        }
        fprintf(stderr, "scene_3d: failed to load shader pair from %s/%s.\n", shader_root, backend);
        return false;
    }

    scene->program = bgfx_create_program(vertex_shader, fragment_shader, true);
    if (!program_handle_valid(scene->program)) {
        fputs("scene_3d: bgfx_create_program failed.\n", stderr);
        scene_3d_shutdown(scene);
        return false;
    }

    if (!encode_cube_mesh(&scene->vertex_buffer, &scene->index_buffer)) {
        fputs("scene_3d: failed to build cube mesh buffers.\n", stderr);
        scene_3d_shutdown(scene);
        return false;
    }

    scene->initialized = true;
    return true;
}

void scene_3d_frame(Scene3D *scene) {
    if (scene == NULL || !scene->initialized) {
        return;
    }

    submit_block(scene->program, scene->vertex_buffer, scene->index_buffer, scene->model);
}
