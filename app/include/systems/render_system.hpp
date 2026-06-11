#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "graphics/graphics_context.hpp"

namespace exd {
namespace systems {

class RenderSystem {
public:
    RenderSystem(graphics::GraphicsContext& graphicsContext);
    ~RenderSystem();

    void update(entities::Registry& registry, const core::Window& window, float dt);

private:
    graphics::GraphicsContext& graphics_context_;
};

} // namespace systems
} // namespace exd
