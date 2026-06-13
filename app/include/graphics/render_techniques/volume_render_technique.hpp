#pragma once
#include "graphics/render_techniques/irender_pass.hpp"
#include "graphics/render_techniques/volume_draw_data.hpp"
#include "graphics/graphics_context.hpp"
#include <glad/gl.h>
#include <cstdint>

namespace exd {
namespace graphics {
namespace render_techniques {

class VolumeRenderTechnique : public IRenderPass {
public:
    explicit VolumeRenderTechnique(graphics::GraphicsContext& ctx);

    void bind()   override;
    void draw(const VolumeDrawData& data);
    void unbind() override;

private:
    graphics::GraphicsContext& ctx_;
    uint32_t program_ = 0;
};

} // namespace render_techniques
} // namespace graphics
} // namespace exd
