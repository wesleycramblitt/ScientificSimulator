#include "common/macros.hpp"
#include "systems/render_system.hpp"
#include "components/transform.hpp"
#include "components/camera.hpp"
#include "components/renderable.hpp"
#include "components/cubemap.hpp"

#include "core/window.hpp"
#include "math/mat4.hpp"
#include "math/quat.hpp"

#include <glad/gl.h>
#include <stdexcept>
#include <iostream>

RenderSystem::RenderSystem(TextureManager* textureManager, MeshManager* meshManager)  : texture_manager_(textureManager), mesh_manager_(meshManager)
{}
RenderSystem::~RenderSystem() {
}

void RenderSystem::update(Registry& registry, const Window& window, float dt) {

    // std::cout << " render system update start " << std::endl;
    Entity camera_entity{};
    Camera* cam = nullptr;
    Transform* cam_xform = nullptr;

    for (auto e : registry.view<Camera, Transform>()) {
        auto& c = registry.get<Camera>(e);
        camera_entity = e;
        cam = &c;
        cam_xform = &registry.get<Transform>(e);
        break;
    }

    if (!cam || !cam_xform) {
        throw std::runtime_error("No camera or camera transform in scene.");
    }

    int width, height;
    float aspect;

    window.getDimensions(width,height,aspect);

    Mat4 view = Mat4::lookAt(
        cam_xform->position,
        cam_xform->position + cam_xform->rotation.forward(),
        cam_xform->rotation.up()
    );

    Mat4 proj = Mat4::perspective(cam->fov_y_radians, aspect, cam->near_plane, cam->far_plane);


    uint32_t cubemap_program = shader_manager_.getOrLoad(
            "cubemap",
            "shaders/cubemap/cubemap.vert",
            "shaders/cubemap/cubemap.frag"
            );


    GL_CALL(glDepthFunc(GL_LEQUAL));
    GL_CALL(glDepthMask(GL_FALSE));
    GL_CALL(glDisable(GL_CULL_FACE));

    GL_CALL(glUseProgram(cubemap_program));

    const GLint u_skybox = glGetUniformLocation(cubemap_program, "u_skybox"); 

    // std::cout << "u_skybox: " << u_skybox << std::endl;

    GLint u_view = glGetUniformLocation(cubemap_program, "u_view");
    GLint u_proj = glGetUniformLocation(cubemap_program, "u_proj");

    GL_CALL(glUniformMatrix4fv(u_view, 1, GL_FALSE, view.m)); 
    GL_CALL(glUniformMatrix4fv(u_proj, 1, GL_FALSE, proj.m));

    for (auto e: registry.view<CubeMap, Renderable>()) {
        auto& cubeMap = registry.get<CubeMap>(e);
        auto& renderable = registry.get<Renderable>(e);
        
        if (renderable.mesh == 0) continue;

        // std::cout << "about to bind textures from cubemap: "<< cubeMap.texture_handle << std::endl;
        const TextureGPU* textureGPU = texture_manager_->bind(cubeMap.texture_handle);

        // std::cout << "glUiniform1i call" << std::endl;
        GL_CALL(glUniform1i(u_skybox, 0));
        // std::cout << "bind mesh" << std::endl; 
        const MeshGPU* mesh =  mesh_manager_->bind(renderable.mesh);
       
        GL_CALL(glDrawArrays(mesh->topology, 0, (GLsizei)mesh->vertex_count));
        GL_CALL(glBindVertexArray(0));
        GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
    }
    
    GL_CALL(glDepthFunc(GL_LESS));
    GL_CALL(glDepthMask(GL_TRUE));
    GL_CALL(glEnable(GL_CULL_FACE));


    uint32_t mesh_program_ = shader_manager_.getOrLoad(
        "mesh_basic",
        "shaders/common/mesh_basic.vert",
        "shaders/common/mesh_basic.frag"
    );

    GL_CALL(glUseProgram(mesh_program_));

    u_view = glGetUniformLocation(mesh_program_, "u_view");
    u_proj = glGetUniformLocation(mesh_program_, "u_proj");
    const GLint u_model =glGetUniformLocation(mesh_program_, "u_model");

    // std::cout << "view: " << u_view << std::endl;
    // view.print();
    // std::cout << "proj: " << u_proj <<  std::endl;
    // proj.print();
    GL_CALL(glUniformMatrix4fv(u_view, 1, GL_FALSE, view.m)); 
    GL_CALL(glUniformMatrix4fv(u_proj, 1, GL_FALSE, proj.m));
    
    for (auto e : registry.view<Transform, Renderable>()) {
        auto& transform = registry.get<Transform>(e);
        auto& renderable = registry.get<Renderable>(e);

        if (renderable.mesh == 0) continue;

        Mat4 model = Mat4::modelTRS(transform.position, transform.rotation, transform.scale);

        // std::cout << "model: " << u_model << std::endl;
        // model.print();

        GL_CALL(glUniformMatrix4fv(u_model, 1, GL_FALSE, model.m));

        // std::cout << "renderable mesh handle: " << renderable.mesh << std::endl;
        const MeshGPU* mesh =  mesh_manager_->bind(renderable.mesh);

        //std::cout << "index count: " << mesh->index_count << std::endl;
        
        if (mesh->index_count > 0) {
            GL_CALL(glDrawElements(mesh->topology, (GLsizei)mesh->index_count, GL_UNSIGNED_INT, nullptr));
        } else {
            GL_CALL(glDrawArrays(mesh->topology, 0, (GLsizei)mesh->vertex_count));
        }

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) std::cout << "GL error: " << err << "\n";
    }

}
