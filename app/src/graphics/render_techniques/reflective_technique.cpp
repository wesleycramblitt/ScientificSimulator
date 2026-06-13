#include "graphics/render_techniques/reflective_technique.hpp"
#include "common/macros.hpp"

namespace exd {
namespace graphics {
namespace render_techniques {

ReflectiveTechnique::ReflectiveTechnique(graphics::GraphicsContext& ctx) : ctx_(ctx) {}

void ReflectiveTechnique::bind(const math::Mat4& view, const math::Mat4& proj,
                                const math::Vec3& cam_pos, uint32_t cubemap_handle) {
    if (cubemap_handle == 0) return;
    program_ = ctx_.shader_manager.getOrLoad(
        "reflective", "shaders/reflective/reflective.vert", "shaders/reflective/reflective.frag");
    GL_CALL(glUseProgram(program_));
    u_view_    = glGetUniformLocation(program_, "u_view");
    u_proj_    = glGetUniformLocation(program_, "u_proj");
    u_model_   = glGetUniformLocation(program_, "u_model");
    u_cam_pos_ = glGetUniformLocation(program_, "u_camPos");
    u_skybox_  = glGetUniformLocation(program_, "u_skybox");
    GL_CALL(glUniformMatrix4fv(u_view_, 1, GL_FALSE, view.m));
    GL_CALL(glUniformMatrix4fv(u_proj_, 1, GL_FALSE, proj.m));
    GL_CALL(glUniform3f(u_cam_pos_, cam_pos.x, cam_pos.y, cam_pos.z));
    ctx_.texture_manager.bind(cubemap_handle);
    GL_CALL(glUniform1i(u_skybox_, 0));
}

void ReflectiveTechnique::draw(uint32_t mesh_handle, const math::Mat4& model) {
    if (mesh_handle == 0) return;
    GL_CALL(glUniformMatrix4fv(u_model_, 1, GL_FALSE, model.m));
    const graphics::MeshGPU* mesh = ctx_.mesh_manager.bind(mesh_handle);
    if (mesh->index_count > 0)
        GL_CALL(glDrawElements(mesh->topology, (GLsizei)mesh->index_count, GL_UNSIGNED_INT, nullptr));
    else
        GL_CALL(glDrawArrays(mesh->topology, 0, (GLsizei)mesh->vertex_count));
}

void ReflectiveTechnique::unbind() {
    GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
    GL_CALL(glUseProgram(0));
}

} // namespace render_techniques
} // namespace graphics
} // namespace exd
