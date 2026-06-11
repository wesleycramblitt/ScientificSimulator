#pragma once
#include "graphics/mesh_manager.hpp"
#include "graphics/texture_manager.hpp"
#include "graphics/shader_manager.hpp"

namespace exd {
namespace graphics {

struct GraphicsContext {
   MeshManager  mesh_manager;
   ShaderManager shader_manager;
   TextureManager texture_manager;
};

} // namespace graphics
} // namespace exd
