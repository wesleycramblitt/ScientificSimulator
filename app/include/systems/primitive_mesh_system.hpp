#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "graphics/graphics_context.hpp"
#include "components/cube.hpp"

namespace exd {
namespace systems {

class PrimitiveMeshSystem {
    public:
        PrimitiveMeshSystem(graphics::GraphicsContext& graphicsContext);
        void update(entities::Registry& registry, core::Window& window);
        graphics::Mesh createMesh(components::Cube cube);
    private:
        graphics::GraphicsContext& graphicsContext_;
};

} // namespace systems
} // namespace exd
