
#include "systems/cubemap_system.hpp"
#include "core/window.hpp"
#include "components/renderable.hpp"


CubeMapSystem::CubeMapSystem(MeshManager* _meshManager, TextureManager* _textureManager) :
    meshManager_(_meshManager), textureManager_(_textureManager) {}

void CubeMapSystem::update(Registry& registry, Window& window) {
 
    for (auto e : registry.view<CubeMap>()) {
        auto& cube = registry.get<CubeMap>(e);

        setCubeMapTextures(cube);

        std::cout << " uploading cubemap textures... " << std::endl;
        cube.texture_handle = textureManager_->uploadToGPU(cube); 

        auto mesh = createMesh(cube);
        
        std::cout << "upload cubemap mesh" << std:: endl;
        int32_t mesh_handle = meshManager_->create(mesh);

        registry.emplace<Renderable>(e, mesh_handle);

        std::cout << "done with cube map syste." << std::endl;
        
    }
}

void CubeMapSystem::setCubeMapTextures(CubeMap& cubemap) {
    cubemap.faces = {
        { "assets/cubemaps/"+cubemap.name+"/1.bmp" },
        { "assets/cubemaps/"+cubemap.name+"/2.bmp" },
        { "assets/cubemaps/"+cubemap.name+"/3.bmp" },
        { "assets/cubemaps/"+cubemap.name+"/4.bmp" },
        { "assets/cubemaps/"+cubemap.name+"/5.bmp" },
        { "assets/cubemaps/"+cubemap.name+"/6.bmp" }
    };
}


Mesh CubeMapSystem::createMesh(CubeMap cubemap)
{
    Mesh mesh;

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
        Vertex v;
        v.position = { vertices[i + 0], vertices[i+1], vertices[i+2] } ;
        mesh.vertices.push_back(v);
    }

    return mesh;
}

