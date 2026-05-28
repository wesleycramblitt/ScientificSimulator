#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "graphics/mesh_manager.hpp"
#include "components/grid.hpp"

class GridSystem {
public:
    explicit GridSystem(MeshManager* meshManager);
    void update(Registry& registry, Window& window);

private:
    Mesh createMesh(const Grid& grid);

    MeshManager* meshManager_;
    uint32_t last_mesh_handle_ = 0;
};
