#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "graphics/graphics_context.hpp"
#include "components/cube.hpp"
#include <unordered_map>

namespace exd {
namespace systems {

class PrimitiveMeshSystem {
    public:
        PrimitiveMeshSystem(graphics::GraphicsContext& graphicsContext);
        void update(entities::Registry& registry, core::Window& window);
        graphics::Mesh createMesh(components::Cube cube);
    private:
        graphics::GraphicsContext& graphicsContext_;
        // Cache last-known size per entity so we only regenerate on change
        std::unordered_map<entities::Entity::id_type, float> cube_size_cache_;
};

} // namespace systems
} // namespace exd
