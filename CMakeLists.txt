cmake_minimum_required(VERSION 3.20)

project(Game VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(USE_SDL2 "Build using SDL2 + bgfx instead of X11 fallback" ON)

# Try SDL2 via CMake package first (best for vcpkg/Windows), then pkg-config.
set(SDL2_TARGET "")
find_package(SDL2 CONFIG QUIET)
if(TARGET SDL2::SDL2)
    set(SDL2_TARGET SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    set(SDL2_TARGET SDL2::SDL2-static)
endif()

if(NOT SDL2_TARGET)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
        if(TARGET PkgConfig::SDL2)
            set(SDL2_TARGET PkgConfig::SDL2)
        endif()
    endif()
endif()

# If SDL2 is missing, keep local development unblocked with X11 fallback.
if(USE_SDL2 AND NOT SDL2_TARGET)
    message(WARNING "SDL2 requested but not found. Falling back to X11. Install SDL2 dev package and reconfigure to enable SDL2+bgfx.")
    set(USE_SDL2 OFF)
endif()

include(FetchContent)

set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    bgfx_cmake
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    GIT_TAG master
)
FetchContent_MakeAvailable(bgfx_cmake)

set(BGFX_TARGET "")
if(TARGET bgfx)
    set(BGFX_TARGET bgfx)
elseif(TARGET bgfx::bgfx)
    set(BGFX_TARGET bgfx::bgfx)
endif()

if(NOT BGFX_TARGET)
    message(FATAL_ERROR "Failed to resolve bgfx target from bgfx.cmake.")
endif()

function(apply_game_warning_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra -Wpedantic> $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic>)
    endif()
endfunction()

add_library(game_shared STATIC
    src/renderer/bgfx_renderer.c
    src/ui/start_menu.c
    src/app/game_app.c
    src/environment/camera_3d.c
    src/environment/scene_3d.c
    src/environment/environment_3d.c
)

target_include_directories(game_shared PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(game_shared PUBLIC ${BGFX_TARGET} bx)
apply_game_warning_options(game_shared)

if(USE_SDL2)
    add_executable(game
        src/main_sdl.c
        src/sdl_window.c
    )

    target_link_libraries(game PRIVATE game_shared ${SDL2_TARGET})
    # bgfx is C++, so force C++ linker even though our app sources are C.
    set_target_properties(game PROPERTIES LINKER_LANGUAGE CXX)
else()
    find_package(X11 REQUIRED)

    add_executable(game
        src/main.c
    )

    target_link_libraries(game PRIVATE game_shared X11::X11)
    set_target_properties(game PROPERTIES LINKER_LANGUAGE CXX)
endif()

apply_game_warning_options(game)
