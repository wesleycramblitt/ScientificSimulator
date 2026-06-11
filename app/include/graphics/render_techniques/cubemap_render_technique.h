#pragma once

#include "graphics/render_techniques/Irender_technique.h"
#include "graphics/render_techniques/renderable.hpp"
#include "graphics/graphics_context.hpp"
#include "math/mat4.hpp"

namespace exd {
namespace graphics {
namespace render_techniques {

class CubeMapRenderTechnique : IRenderTechnique {
    uint32_t cubemap_program_; 

    public:
    CubeMapRenderTechnique(graphics::GraphicsContext& graphicsContext) : IRenderTechnique(graphicsContext) {}
    
    void bind() override;
    void draw(const Renderable& renderable) override;
    void unbind() override; 
};

} // namespace render_techniques
} // namespace graphics
} // namespace exd
