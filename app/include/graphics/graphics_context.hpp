#pragma once
#include "graphics/mesh_manager.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/shader_manager.hpp"

 
struct GraphicsContext {
   MeshManager&  meshManager;
   ShaderManager& shaderManager;
};
