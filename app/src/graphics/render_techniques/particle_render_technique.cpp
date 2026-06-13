#include "graphics/render_techniques/particle_render_technique.hpp"
#include "common/macros.hpp"
#include <cstdio>
#include <cstring>
#include <vector>

namespace exd {
namespace graphics {
namespace render_techniques {

ParticleRenderTechnique::ParticleRenderTechnique(graphics::GraphicsContext& ctx)
    : ctx_(ctx) {}

void ParticleRenderTechnique::initGL(GLState& s, int capacity) {
    GL_CALL(glGenVertexArrays(1, &s.vao));
    GL_CALL(glGenBuffers(1, &s.vbo));
    GL_CALL(glBindVertexArray(s.vao));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, s.vbo));
    // 6 floats per particle: x,y,z  r,g,b
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, (size_t)capacity * 6 * sizeof(float),
                         nullptr, GL_DYNAMIC_DRAW));
    GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0));
    GL_CALL(glEnableVertexAttribArray(0));
    GL_CALL(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float))));
    GL_CALL(glEnableVertexAttribArray(1));
    GL_CALL(glBindVertexArray(0));
    s.capacity = capacity;
}

void ParticleRenderTechnique::upload(GLState& s,
                                      const float* positions,
                                      const float* colors, int count) {
    // Interleave position + color into a single upload
    std::vector<float> interleaved(count * 6);
    for (int i = 0; i < count; ++i) {
        interleaved[i*6 + 0] = positions[i*3 + 0];
        interleaved[i*6 + 1] = positions[i*3 + 1];
        interleaved[i*6 + 2] = positions[i*3 + 2];
        if (colors) {
            interleaved[i*6 + 3] = colors[i*3 + 0];
            interleaved[i*6 + 4] = colors[i*3 + 1];
            interleaved[i*6 + 5] = colors[i*3 + 2];
        } else {
            // fallback: orange
            interleaved[i*6 + 3] = 1.0f;
            interleaved[i*6 + 4] = 0.6f;
            interleaved[i*6 + 5] = 0.1f;
        }
    }
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, s.vbo));
    GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, 0, count * 6 * sizeof(float), interleaved.data()));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void ParticleRenderTechnique::bind() {
    program_ = ctx_.shader_manager.getOrLoad(
        "particle_points",
        "shaders/particle/particle.vert",
        "shaders/particle/particle.frag");
    GL_CALL(glUseProgram(program_));
    GL_CALL(glEnable(GL_PROGRAM_POINT_SIZE));  // let vertex shader control point size
}

void ParticleRenderTechnique::draw(const ParticleDrawData& data) {
    if (!data.positions || data.count == 0) return;

    if (state_.vao == 0 || state_.capacity < data.count) {
        if (state_.vao) { GL_CALL(glDeleteVertexArrays(1, &state_.vao)); GL_CALL(glDeleteBuffers(1, &state_.vbo)); }
        initGL(state_, data.count);
    }
    upload(state_, data.positions, data.colors, data.count);

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

    GL_CALL(glBindVertexArray(state_.vao));
    GL_CALL(glEnable(GL_BLEND));
    GL_CALL(glBlendFunc(GL_ONE, GL_ONE));  // additive — overlapping particles glow brighter
    GL_CALL(glDepthMask(GL_FALSE));
    GL_CALL(glDrawArrays(GL_POINTS, 0, data.count));
    GL_CALL(glBindVertexArray(0));
}

void ParticleRenderTechnique::unbind() {
    GL_CALL(glDepthMask(GL_TRUE));
    GL_CALL(glDisable(GL_BLEND));
    GL_CALL(glUseProgram(0));
}

} // namespace render_techniques
} // namespace graphics
} // namespace exd
