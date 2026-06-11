
#include "systems/cubemap_system.hpp"
#include "core/window.hpp"
#include "components/renderable.hpp"

namespace exd {
namespace systems {

CubeMapSystem::CubeMapSystem(graphics::GraphicsContext& graphicsContext) :
    graphicsContext_(graphicsContext){}

void CubeMapSystem::update(entities::Registry& registry, core::Window& window) {
 
    for (auto e : registry.view<components::CubeMap>()) {
        auto& cube = registry.get<components::CubeMap>(e);

        setCubeMapTextures(cube);

        cube.texture_handle = graphicsContext_.texture_manager.uploadToGPU(cube); 

        auto mesh = createMesh(cube);
        
        int32_t mesh_handle = graphicsContext_.mesh_manager.create(mesh);

        registry.emplace<components::Renderable>(e, mesh_handle);

        
    }
}

void CubeMapSystem::setCubeMapTextures(components::CubeMap& cubemap) {
    if (cubemap.cross_layout) {
        // Single cross-shaped image
        cubemap.faces = {
            { "assets/cubemaps/"+cubemap.name+"/cross.png" }
        };
    } else {
        // Individual face files
        cubemap.faces = {
            { "assets/cubemaps/"+cubemap.name+"/1.bmp" },
            { "assets/cubemaps/"+cubemap.name+"/2.bmp" },
            { "assets/cubemaps/"+cubemap.name+"/3.bmp" },
            { "assets/cubemaps/"+cubemap.name+"/4.bmp" },
            { "assets/cubemaps/"+cubemap.name+"/5.bmp" },
            { "assets/cubemaps/"+cubemap.name+"/6.bmp" }
        };
    }
}


graphics::Mesh CubeMapSystem::createMesh(components::CubeMap cubemap)
{
    graphics::Mesh mesh;

    float vertices[] = {
        // positions
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };


    constexpr std::size_t stride = 3;
    constexpr std::size_t count = sizeof(vertices) / sizeof(float);
    for (std::size_t i{}; i < count; i+= stride) {
        graphics::Vertex v;
        v.position = { vertices[i + 0], vertices[i+1], vertices[i+2] } ;
        mesh.vertices.push_back(v);
    }

    return mesh;
}

} // namespace systems
} // namespace exd
