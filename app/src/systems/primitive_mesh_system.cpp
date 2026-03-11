#include "systems/primitive_mesh_system.hpp"
#include "core/window.hpp"
#include "components/sphere.hpp"
#include "components/cube.hpp"
#include "components/renderable.hpp"


PrimitiveMeshSystem::PrimitiveMeshSystem(MeshManager* _meshManager) : meshManager_(_meshManager) {}

void PrimitiveMeshSystem::update(Registry& registry, Window& window) {
 
    for (auto e : registry.view<Cube>()) {
        auto cube = registry.get<Cube>(e);
        auto mesh = createMesh(cube);
        
        int32_t mesh_handle = meshManager_->create(mesh);
        registry.emplace<Renderable>(e, mesh_handle);

        
    }
}

Mesh PrimitiveMeshSystem::createMesh(Cube cube)
{
    Mesh mesh;

    float h = cube.size * 0.5f;

    // Positions per face
    struct Face {
        Vec3 normal;
        Vec3 v0, v1, v2, v3;
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


