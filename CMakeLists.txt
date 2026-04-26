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

if(USE_SDL2)
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

    add_executable(game
        src/main_sdl.c
        src/sdl_window.c
        src/renderer/bgfx_renderer.c
        src/environment/environment_3d.cpp
    )

    target_include_directories(game PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_definitions(game PRIVATE USE_SDL2)
    target_compile_options(game PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(game PRIVATE ${SDL2_TARGET} ${BGFX_TARGET} bx)
    # bgfx is C++, so force C++ linker even though our app sources are C.
    set_target_properties(game PROPERTIES LINKER_LANGUAGE CXX)
else()
    find_package(X11 REQUIRED)

    add_executable(game
        src/main.c
    )

    target_compile_options(game PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(game PRIVATE X11::X11)
endif()
