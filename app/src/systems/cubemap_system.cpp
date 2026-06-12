
#include "systems/cubemap_system.hpp"
#include "core/window.hpp"
#include "components/renderable.hpp"
#include "graphics/texture_cubemap.hpp"
#include <array>

namespace exd {
namespace systems {

CubeMapSystem::CubeMapSystem(graphics::GraphicsContext& graphicsContext) :
    graphicsContext_(graphicsContext){}

void CubeMapSystem::update(entities::Registry& registry, core::Window& window) {
 
    for (auto e : registry.view<components::CubeMap>()) {
        auto& cube = registry.get<components::CubeMap>(e);

        // Build a CubeMapTexture from the component data and upload
        if (cube.cross_layout) {
            graphics::CubeMapTexture tex(
                "assets/cubemaps/" + cube.name + "/cross.png",
                512  // face_size, will be recalculated in upload_level
            );
            cube.texture_handle = graphicsContext_.texture_manager.uploadToGPU(tex);
        } else {
            std::array<std::string, 6> face_paths = {
                "assets/cubemaps/" + cube.name + "/1.bmp",
                "assets/cubemaps/" + cube.name + "/2.bmp",
                "assets/cubemaps/" + cube.name + "/3.bmp",
                "assets/cubemaps/" + cube.name + "/4.bmp",
                "assets/cubemaps/" + cube.name + "/5.bmp",
                "assets/cubemaps/" + cube.name + "/6.bmp",
            };
            graphics::CubeMapTexture tex(std::move(face_paths));
            cube.texture_handle = graphicsContext_.texture_manager.uploadToGPU(tex);
        }

        auto mesh = createMesh(cube);
        
        int32_t mesh_handle = graphicsContext_.mesh_manager.create(mesh);

        registry.emplace<components::Renderable>(e, mesh_handle);

        
    }
}


graphics::Mesh CubeMapSystem::createMesh(components::CubeMap /*cubemap*/)
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
