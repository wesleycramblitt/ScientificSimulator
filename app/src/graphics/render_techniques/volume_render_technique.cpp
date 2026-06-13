#include "graphics/render_techniques/volume_render_technique.hpp"
#include "common/macros.hpp"

namespace exd {
namespace graphics {
namespace render_techniques {

VolumeRenderTechnique::VolumeRenderTechnique(graphics::GraphicsContext& ctx)
    : ctx_(ctx) {}

void VolumeRenderTechnique::bind() {
    program_ = ctx_.shader_manager.getOrLoad(
        "volume_ray",
        "shaders/volume/ray_march.vert",
        "shaders/volume/ray_march.frag");
    GL_CALL(glUseProgram(program_));
}

void VolumeRenderTechnique::draw(const VolumeDrawData& data) {
    if (data.texture_handle == 0 || data.proxy_mesh == 0) return;

    // Apply uniforms
    for (const auto& [name, value] : data.uniforms) {
        GLint loc = glGetUniformLocation(program_, name.c_str());
        std::visit([loc](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, int>)
                GL_CALL(glUniform1i(loc, v));
            else if constexpr (std::is_same_v<T, float>)
                GL_CALL(glUniform1f(loc, v));
            else if constexpr (std::is_same_v<T, math::Vec3>)
                GL_CALL(glUniform3f(loc, v.x, v.y, v.z));
            else if constexpr (std::is_same_v<T, math::Mat4>)
                GL_CALL(glUniformMatrix4fv(loc, 1, GL_FALSE, v.m));
        }, value);
    }

    // Bind 3D texture
    ctx_.texture_manager.bind(data.texture_handle);
    GL_CALL(glUniform1i(glGetUniformLocation(program_, "u_volume"), 0));

    // Grid dimensions (ivec3)
    GL_CALL(glUniform3i(glGetUniformLocation(program_, "u_grid_dims"),
                        data.nx, data.ny, data.nz));

    // Draw proxy geometry
    GL_CALL(glEnable(GL_CULL_FACE));
    GL_CALL(glCullFace(GL_FRONT));

    const graphics::MeshGPU* mesh = ctx_.mesh_manager.bind(data.proxy_mesh);
    GL_CALL(glDrawArrays(mesh->topology, 0, (GLsizei)mesh->vertex_count));

    GL_CALL(glCullFace(GL_BACK));
    GL_CALL(glBindTexture(GL_TEXTURE_3D, 0));
}

void VolumeRenderTechnique::unbind() {
    GL_CALL(glUseProgram(0));
}

} // namespace render_techniques
} // namespace graphics
} // namespace exd
