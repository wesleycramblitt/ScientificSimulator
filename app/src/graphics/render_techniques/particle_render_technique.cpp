#include "graphics/render_techniques/particle_render_technique.hpp"
#include "common/macros.hpp"
#include <cstdio>

namespace exd {
namespace graphics {
namespace render_techniques {

ParticleRenderTechnique::ParticleRenderTechnique(graphics::GraphicsContext& ctx)
    : ctx_(ctx) {}

void ParticleRenderTechnique::initGL(GLState& s, int particle_count) {
    GL_CALL(glGenVertexArrays(1, &s.vao));
    GL_CALL(glGenBuffers(1, &s.vbo));
    GL_CALL(glBindVertexArray(s.vao));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, s.vbo));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, (size_t)particle_count * 3 * sizeof(float),
                         nullptr, GL_DYNAMIC_DRAW));
    GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0));
    GL_CALL(glEnableVertexAttribArray(0));
    GL_CALL(glBindVertexArray(0));
    s.count = particle_count;
}

void ParticleRenderTechnique::uploadPositions(GLState& s,
                                               const float* positions, int count) {
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, s.vbo));
    GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, 0, count * 3 * sizeof(float), positions));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void ParticleRenderTechnique::bind() {
    program_ = ctx_.shader_manager.getOrLoad(
        "particle_points",
        "shaders/particle/particle.vert",
        "shaders/particle/particle.frag");
    GL_CALL(glUseProgram(program_));
}

void ParticleRenderTechnique::draw(const ParticleDrawData& data) {
    if (!data.positions || data.count == 0) return;

    // Lazy-init or re-init GL state for this call site
    uint32_t key = next_key_++;
    auto& s = per_entity_[key];
    if (s.vao == 0 || s.count < data.count) {
        if (s.vao) { GL_CALL(glDeleteVertexArrays(1, &s.vao)); GL_CALL(glDeleteBuffers(1, &s.vbo)); }
        initGL(s, data.count);
    }
    uploadPositions(s, data.positions, data.count);

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

    GL_CALL(glBindVertexArray(s.vao));
    GL_CALL(glPointSize(2.0f));
    GL_CALL(glDrawArrays(GL_POINTS, 0, data.count));
    GL_CALL(glBindVertexArray(0));
}

void ParticleRenderTechnique::unbind() {
    GL_CALL(glUseProgram(0));
}

} // namespace render_techniques
} // namespace graphics
} // namespace exd
