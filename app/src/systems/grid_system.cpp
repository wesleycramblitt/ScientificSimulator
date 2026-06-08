#include "systems/grid_system.hpp"
#include "components/renderable.hpp"
#include "components/transform.hpp"
#include "components/disabled.hpp"
#include "graphics/vertex.hpp"
#include "math/vec3.hpp"

GridSystem::GridSystem(MeshManager* meshManager) : meshManager_(meshManager) {}

void GridSystem::update(Registry& registry, Window& window) {
    for (auto e : registry.view<Grid, Transform>()) {
        if (registry.has<Disabled>(e)) continue;

        if (window.grid_visible && !registry.has<Renderable>(e)) {
            Mesh mesh = createMesh(registry.get<Grid>(e));
            uint32_t handle = meshManager_->create(mesh);
            registry.emplace<Renderable>(e, handle);
        } else if (!window.grid_visible && registry.has<Renderable>(e)) {
            registry.remove<Renderable>(e);
        }
    }
}

Mesh GridSystem::createMesh(const Grid& grid) {
    Mesh mesh;
    mesh.topology = LINES;

    const float s = grid.spacing > 0.0f ? grid.spacing : 1.0f;
    const int    N = 100;
    const float extent = N * s;

    for (int i = -N; i <= N; ++i) {
        const float coord = i * s;
        
        mesh.vertices.push_back(Vertex{.position = Vec3{-extent, 0.0f, coord}, .color=grid.color });
        mesh.vertices.push_back(Vertex{.position = Vec3{+extent, 0.0f, coord}, .color=grid.color });
        mesh.vertices.push_back(Vertex{.position = Vec3{coord, 0.0f, -extent}, .color=grid.color });
        mesh.vertices.push_back(Vertex{.position = Vec3{coord, 0.0f, +extent}, .color=grid.color });
    }

    return mesh;
}
