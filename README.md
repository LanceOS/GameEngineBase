# Game

SDL2 + bgfx starter for building a custom game without an engine. This first milestone opens a window and runs a game loop that submits bgfx frames.

## Architecture

- `src/main_sdl.c`: game loop orchestration
- `src/sdl_window.c`: SDL2 window/events/native handle layer
- `src/bgfx_renderer.c`: bgfx initialization/frame/resize/shutdown
- `src/main.c`: Linux X11 fallback path when SDL2 is unavailable

## Requirements

- C compiler (`gcc` or `clang`)
- C++ compiler (bgfx is C++)
- CMake 3.20+
- Git (required for CMake `FetchContent` when downloading `bgfx.cmake`)
- SDL2 development package

Install dependencies on common platforms:

Debian / Ubuntu:

```bash
sudo apt update
sudo apt install build-essential cmake git libsdl2-dev libx11-dev
```

Fedora / RHEL (dnf):

```bash
sudo dnf install @development-tools cmake git SDL2-devel libX11-devel
```

macOS (Homebrew):

```bash
brew update
brew install cmake git sdl2
```

Windows (vcpkg + Visual Studio Build Tools):

```powershell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
./vcpkg install sdl2:x64-windows
```

## Build

Default build (uses SDL2+bgfx when SDL2 is found):

```bash
cmake -S . -B build
cmake --build build --config Release
```

Force SDL2+bgfx path:

```bash
cmake -S . -B build -DUSE_SDL2=ON
cmake --build build --config Release
```

If SDL2 is missing, CMake will warn and fall back to the X11 implementation on Linux.

## Run

```bash
./build/bin/game
```

Close the window with the titlebar close button or press `Escape`.

## CI

The repository includes `.github/workflows/ci.yml` with Linux, macOS, and Windows builds.
