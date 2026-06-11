#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "graphics/graphics_context.hpp"
#include "components/mesh_asset.hpp"

namespace exd {
namespace systems {

class MeshAssetSystem {
    public:
        MeshAssetSystem(graphics::GraphicsContext& graphicsContext);
        void update(entities::Registry& registry, core::Window& window);
        graphics::Mesh createMesh(components::MeshAsset meshAsset);
    private:
        graphics::GraphicsContext& graphicsContext_;
};

} // namespace systems
} // namespace exd
