#pragma once
#include "graphics/graphics_context.hpp"
#include "math/mat4.hpp"
#include <glad/gl.h>
#include <cstdint>

namespace exd {
namespace graphics {
namespace render_techniques {

class LambertianTechnique {
public:
    explicit LambertianTechnique(graphics::GraphicsContext& ctx);
    void bind(const math::Mat4& view, const math::Mat4& proj);
    void draw(uint32_t mesh_handle, const math::Mat4& model);
    void unbind();

private:
    graphics::GraphicsContext& ctx_;
    uint32_t program_ = 0;
    GLint u_view_ = -1, u_proj_ = -1, u_model_ = -1, u_light_dir_ = -1;
};

} // namespace render_techniques
} // namespace graphics
} // namespace exd
