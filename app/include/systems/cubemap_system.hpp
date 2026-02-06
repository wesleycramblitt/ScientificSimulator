#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "graphics/mesh_manager.hpp"
#include "components/cubemap.hpp"
#include "graphics/texture_manager.hpp"

class CubeMapSystem {
    public:
        CubeMapSystem(MeshManager* _meshManager, TextureManager* _textureManager);
        void update(Registry& registry,Window& window);
        Mesh createMesh(CubeMap cubemap);
        void setCubeMapTextures(CubeMap& cubemap);
    private:
        MeshManager* meshManager_;
        TextureManager* textureManager_;
};


