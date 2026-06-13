#pragma once
#include "graphics/render_techniques/irender_pass.hpp"
#include "graphics/render_techniques/particle_draw_data.hpp"
#include "graphics/graphics_context.hpp"
#include <unordered_map>
#include <glad/gl.h>
#include <cstdint>

namespace exd {
namespace graphics {
namespace render_techniques {

class ParticleRenderTechnique : public IRenderPass {
public:
    explicit ParticleRenderTechnique(graphics::GraphicsContext& ctx);

    void bind()   override;
    void draw(const ParticleDrawData& data);
    void unbind() override;

private:
    struct GLState {
        GLuint vao = 0;
        GLuint vbo = 0;
        int    capacity = 0;       // allocated size, may be > drawn count
    };

    void initGL(GLState& s, int capacity);
    void upload(GLState& s, const float* positions, const float* colors, int count);

    graphics::GraphicsContext& ctx_;
    uint32_t program_ = 0;
    std::unordered_map<uint32_t, GLState> states_;
    uint32_t next_key_ = 0;
};

} // namespace render_techniques
} // namespace graphics
} // namespace exd
