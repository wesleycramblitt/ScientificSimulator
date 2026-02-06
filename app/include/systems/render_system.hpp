#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "graphics/shader_manager.hpp"
#include "graphics/mesh_manager.hpp"
#include "graphics/texture_manager.hpp"

class RenderSystem {
public:
    RenderSystem(TextureManager* textureManager, MeshManager* meshManager);
    ~RenderSystem();

    void update(Registry& registry, const Window& window, float dt);

private:
    ShaderManager shader_manager_;
    MeshManager*  mesh_manager_;
    TextureManager* texture_manager_;
};
