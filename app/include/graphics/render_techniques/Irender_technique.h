#pragma once

#include "graphics/render_techniques/renderable.hpp"
#include "graphics/graphics_context.hpp"

namespace exd {
namespace graphics {
namespace render_techniques {

class IRenderTechnique {
    public:
        virtual ~IRenderTechnique() = default;
        virtual void bind() = 0;
        virtual void draw(const Renderable& renderable) = 0;
        virtual void unbind() = 0;

    protected:
        IRenderTechnique(graphics::GraphicsContext& graphicsContext)  :  graphicsContext_(graphicsContext) {}
        graphics::GraphicsContext& graphicsContext_;
};

} // namespace render_techniques
} // namespace graphics
} // namespace exd
