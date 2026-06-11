#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "components/cubemap.hpp"
#include "graphics/graphics_context.hpp"

namespace exd {
namespace systems {

class CubeMapSystem {
    public:
        CubeMapSystem(graphics::GraphicsContext& graphicsContext);
        void update(entities::Registry& registry, core::Window& window);
        graphics::Mesh createMesh(components::CubeMap cubemap);
        void setCubeMapTextures(components::CubeMap& cubemap);
    private:
        graphics::GraphicsContext& graphicsContext_;
};

} // namespace systems
} // namespace exd
