#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "graphics/mesh_manager.hpp"
#include "components/mesh_asset.hpp"

class MeshAssetSystem {
    public:
        MeshAssetSystem(MeshManager* _meshManager);
        void update(Registry& registry,Window& window);
        Mesh createMesh(MeshAsset meshAsset);
    private:
        MeshManager* meshManager_;
};
