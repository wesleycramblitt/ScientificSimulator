#include "systems/grid_system.hpp"
#include "components/renderable.hpp"
#include "components/transform.hpp"
#include "components/disabled.hpp"
#include "graphics/vertex.hpp"
#include "math/vec3.hpp"

namespace exd {
namespace systems {

GridSystem::GridSystem(graphics::GraphicsContext& graphicsContext) : graphicsContext_(graphicsContext) {}

void GridSystem::update(entities::Registry& registry, core::Window& window) {
    for (auto e : registry.view<components::Grid, components::Transform>()) {
        if (registry.has<components::Disabled>(e)) continue;

        if (window.grid_visible && !registry.has<components::Renderable>(e)) {
            graphics::Mesh mesh = createMesh(registry.get<components::Grid>(e));
            uint32_t handle = graphicsContext_.mesh_manager.create(mesh);
            registry.emplace<components::Renderable>(e, handle);
        } else if (!window.grid_visible && registry.has<components::Renderable>(e)) {
            registry.remove<components::Renderable>(e);
        }
    }
}

graphics::Mesh GridSystem::createMesh(const components::Grid& grid) {
    graphics::Mesh mesh;
    mesh.topology = graphics::LINES;

    const float s = grid.spacing > 0.0f ? grid.spacing : 1.0f;
    const int    N = 10;
    const float extent = N * s;

    for (int i = -N; i <= N; ++i) {
        const float coord = i * s;
        
        mesh.vertices.push_back(graphics::Vertex{.position = math::Vec3{-extent, 0.0f, coord}, .color=grid.color });
        mesh.vertices.push_back(graphics::Vertex{.position = math::Vec3{+extent, 0.0f, coord}, .color=grid.color });
        mesh.vertices.push_back(graphics::Vertex{.position = math::Vec3{coord, 0.0f, -extent}, .color=grid.color });
        mesh.vertices.push_back(graphics::Vertex{.position = math::Vec3{coord, 0.0f, +extent}, .color=grid.color });
    }

    return mesh;
}

} // namespace systems
} // namespace exd
