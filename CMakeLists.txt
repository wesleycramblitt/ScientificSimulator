
cmake_minimum_required(VERSION 3.16)
project(ScientificSimulator LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-fdiagnostics-color=always)

include(FetchContent)

# -------------------------
# SDL3: system-first, fallback to FetchContent
# -------------------------
find_package(SDL3 CONFIG QUIET)

if (NOT SDL3_FOUND)
  message(STATUS "SDL3 not found via system packages; fetching SDL3...")
  FetchContent_Declare(
      SDL3
      GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
      GIT_TAG        release-3.2.18
  )
  FetchContent_MakeAvailable(SDL3)
endif()

# -------------------------
# OpenGL: cross-platform via CMake targets
# -------------------------
find_package(OpenGL REQUIRED)

add_executable(ScientificSimulator
  app/main.cpp
  app/src/core/app.cpp
  app/src/core/window.cpp
  app/src/core/event_state.cpp
  app/src/entities/registry.cpp
  app/src/scene/scene_manager.cpp
  app/src/systems/camera_controller_system.cpp
  app/src/systems/render_system.cpp
  external/glad/src/gl.c
)

target_include_directories(ScientificSimulator PRIVATE
  app/include
  external/glad/include
)

# SDL3
target_link_libraries(ScientificSimulator PRIVATE SDL3::SDL3)
target_compile_definitions(ScientificSimulator PRIVATE SDL_MAIN_USE_CALLBACKS=0)

# OpenGL linkage
if (WIN32)
  target_link_libraries(ScientificSimulator PRIVATE opengl32)
elseif (APPLE)
  # find_package(OpenGL) typically provides OpenGL::GL on macOS,
  # but some setups still need the framework explicitly.
  find_library(COCOA_LIBRARY Cocoa REQUIRED)
  target_link_libraries(ScientificSimulator PRIVATE ${COCOA_LIBRARY} OpenGL::GL)
else()
  # Linux: prefer GLVND through OpenGL::GL (works for Mesa/NVIDIA)
  target_link_libraries(ScientificSimulator PRIVATE OpenGL::GL)

  # If you *explicitly* use EGL instead of GLX, use this instead:
  # target_link_libraries(ScientificSimulator PRIVATE OpenGL::OpenGL OpenGL::EGL)
endif()
