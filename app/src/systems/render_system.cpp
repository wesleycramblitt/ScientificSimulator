#include "systems/render_system.hpp"
#include "components/transform.hpp"
#include "components/camera.hpp"
#include "components/renderable.hpp"

#include "core/window.hpp"
#include "math/mat4.hpp"
#include "math/quat.hpp"

#include <glad/gl.h>
#include <stdexcept>

RenderSystem::RenderSystem() {}
RenderSystem::~RenderSystem() {
}

void RenderSystem::update(Registry& registry, const Window& window, float /*dt*/) {

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

    mesh_program_ = shader_manager_.getOrLoad(
        "mesh_basic",
        "assets/shaders/mesh_basic.vert",
        "assets/shaders/mesh_basic.frag"
    );

    glUseProgram(mesh_program_);

    const GLint u_view = glGetUniformLocation(mesh_program_, "u_view");
    const GLint u_proj = glGetUniformLocation(mesh_program_, "u_proj");
    const GLint u_model = glGetUniformLocation(mesh_program_, "u_model");

    glUniformMatrix4fv(u_view, 1, GL_FALSE, view.m); 
    glUniformMatrix4fv(u_proj, 1, GL_FALSE, proj.m);

    for (auto e : registry.view<Transform, Renderable>()) {
        auto& transform = registry.get<Transform>(e);
        auto& renderable = registry.get<Renderable>(e);

        if (renderable.mesh == 0) continue;

        Mat4 model = Mat4::modelTRS(transform.position, transform.rotation, transform.scale);

        glUniformMatrix4fv(u_model, 1, GL_FALSE, model.m);

        MeshGPU mesh =  mesh_manager_.bind(renderable.mesh);

        if (mesh.index_count > 0) {
            glDrawElements(mesh.topology, (GLsizei)mesh.index_count, GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(mesh.topology, 0, (GLsizei)mesh.vertex_count);
        }
    }

}
