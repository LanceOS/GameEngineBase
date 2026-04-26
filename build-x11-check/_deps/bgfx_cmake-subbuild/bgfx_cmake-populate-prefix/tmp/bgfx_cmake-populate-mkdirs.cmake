# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/lance/Documents/Code/Game/build-x11-check/_deps/bgfx_cmake-src")
  file(MAKE_DIRECTORY "/home/lance/Documents/Code/Game/build-x11-check/_deps/bgfx_cmake-src")
endif()
file(MAKE_DIRECTORY
  "/home/lance/Documents/Code/Game/build-x11-check/_deps/bgfx_cmake-build"
  "/home/lance/Documents/Code/Game/build-x11-check/_deps/bgfx_cmake-subbuild/bgfx_cmake-populate-prefix"
  "/home/lance/Documents/Code/Game/build-x11-check/_deps/bgfx_cmake-subbuild/bgfx_cmake-populate-prefix/tmp"
  "/home/lance/Documents/Code/Game/build-x11-check/_deps/bgfx_cmake-subbuild/bgfx_cmake-populate-prefix/src/bgfx_cmake-populate-stamp"
  "/home/lance/Documents/Code/Game/build-x11-check/_deps/bgfx_cmake-subbuild/bgfx_cmake-populate-prefix/src"
  "/home/lance/Documents/Code/Game/build-x11-check/_deps/bgfx_cmake-subbuild/bgfx_cmake-populate-prefix/src/bgfx_cmake-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/lance/Documents/Code/Game/build-x11-check/_deps/bgfx_cmake-subbuild/bgfx_cmake-populate-prefix/src/bgfx_cmake-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/lance/Documents/Code/Game/build-x11-check/_deps/bgfx_cmake-subbuild/bgfx_cmake-populate-prefix/src/bgfx_cmake-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
