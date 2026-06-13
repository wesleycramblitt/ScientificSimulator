#include "systems/primitive_mesh_system.hpp"
#include "core/window.hpp"
#include "components/sphere.hpp"
#include "components/cube.hpp"
#include "components/renderable.hpp"

namespace exd {
namespace systems {

PrimitiveMeshSystem::PrimitiveMeshSystem(graphics::GraphicsContext& graphicsContext) : graphicsContext_(graphicsContext) {}

void PrimitiveMeshSystem::update(entities::Registry& registry, core::Window& window) {
 
    for (auto e : registry.view<components::Cube>()) {
        auto& cube = registry.get<components::Cube>(e);

        // Only regenerate mesh when size actually changes
        auto it = cube_size_cache_.find(e.id);
        if (it != cube_size_cache_.end() && it->second == cube.size)
            continue;

        auto mesh = createMesh(cube);
        uint32_t handle = graphicsContext_.mesh_manager.create(mesh);

        if (registry.has<components::Renderable>(e)) {
            registry.get<components::Renderable>(e).mesh = handle;
        } else {
            registry.emplace<components::Renderable>(e, handle);
        }

        cube_size_cache_[e.id] = cube.size;
    }
}

graphics::Mesh PrimitiveMeshSystem::createMesh(components::Cube cube)
{
    graphics::Mesh mesh;

    float h = cube.size * 0.5f;

    // Positions per face
    struct Face {
        math::Vec3 normal;
        math::Vec3 v0, v1, v2, v3;
    };


    Face faces[6] = {
        // +X
        { { 1, 0, 0 },
          {  h, -h, -h },
          {  h,  h, -h },
          {  h,  h,  h },
          {  h, -h,  h } },

        // -X
        { { -1, 0, 0 },
          { -h, -h,  h },
          { -h,  h,  h },
          { -h,  h, -h },
          { -h, -h, -h } },

        // +Y
        { { 0, 1, 0 },
          { -h,  h, -h },
          {  -h,  h, h },
          {  h,  h,  h },
          { h,  h,  -h } },
        // -Y (bottom)
        { { 0, -1, 0 },
          { -h, -h,  h },  // v0
          { -h, -h, -h },  // v1
          {  h, -h, -h },  // v2
          {  h, -h,  h } }, // v3        
       // +Z
        { { 0, 0, 1 },
          { -h, -h,  h },
          {  h, -h,  h },
          {  h,  h,  h },
          { -h,  h,  h } },

        // -Z
        { { 0, 0, -1 },
          { -h, -h, -h },
          { -h,  h, -h },
          {  h,  h, -h },
          {  h, -h, -h } }
    };

    for (int i = 0; i < 6; ++i)
    {
        uint32_t startIndex = mesh.vertices.size();

        mesh.vertices.push_back({ faces[i].v0, faces[i].normal });
        mesh.vertices.push_back({ faces[i].v1, faces[i].normal });
        mesh.vertices.push_back({ faces[i].v2, faces[i].normal });
        mesh.vertices.push_back({ faces[i].v3, faces[i].normal });

        // Two triangles per face
        mesh.indices.push_back(startIndex + 0);
        mesh.indices.push_back(startIndex + 1);
        mesh.indices.push_back(startIndex + 2);

        mesh.indices.push_back(startIndex + 0);
        mesh.indices.push_back(startIndex + 2);
        mesh.indices.push_back(startIndex + 3);
    }

    return mesh;
}


} // namespace systems
} // namespace exd
