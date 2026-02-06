#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "graphics/mesh_manager.hpp"
#include "components/cube.hpp"

class PrimitiveMeshSystem {
    public:
        PrimitiveMeshSystem(MeshManager* _meshManager);
        void update(Registry& registry,Window& window);
        Mesh createMesh(Cube cube);
    private:
        MeshManager* meshManager_;
};
