
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
  external/glad/src/gl.c
)

# Assimp for importing obj, stl, and other files
FetchContent_Declare(
    assimp
    URL "${CMAKE_SOURCE_DIR}/external/assimp/assimp-6.0.4.zip"
)

# Make the fetched build smaller/faster (optional but recommended)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)

# You can also disable some importers/exporters if you want later.

FetchContent_MakeAvailable(assimp)

# FluidX3D-CLI for GPU-accelerated LBM CFD simulation
add_subdirectory(external/fluidx3d)

# Dear ImGui for debug/editor UI (docking branch)
add_subdirectory(external/imgui)
target_include_directories(imgui PUBLIC external/glad/include)
target_link_libraries(imgui PUBLIC SDL3::SDL3 OpenGL::GL)

add_subdirectory(app/src/components)
add_subdirectory(app/src/core)
add_subdirectory(app/src/entities)
add_subdirectory(app/src/graphics)
add_subdirectory(app/src/scene)
add_subdirectory(app/src/systems)
add_subdirectory(app/src/assets)

target_include_directories(ScientificSimulator PRIVATE
  app/include
  external/glad/include
  external/stb_image/include
)


add_custom_command(TARGET ScientificSimulator POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:ScientificSimulator>/assets
)

add_custom_command(TARGET ScientificSimulator POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/shaders
          $<TARGET_FILE_DIR:ScientificSimulator>/shaders
)


target_compile_definitions(ScientificSimulator PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
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


# Assimp
target_link_libraries(ScientificSimulator PRIVATE assimp::assimp)

# FluidX3D
target_link_libraries(ScientificSimulator PRIVATE fluidx3d)

# Dear ImGui
target_link_libraries(ScientificSimulator PRIVATE imgui)

add_executable(test_fluidx3d tests/test_fluidx3d.cpp)
target_link_libraries(test_fluidx3d PRIVATE fluidx3d)

