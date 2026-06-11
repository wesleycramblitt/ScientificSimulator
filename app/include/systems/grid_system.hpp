#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "graphics/graphics_context.hpp"
#include "components/grid.hpp"

namespace exd {
namespace systems {

class GridSystem {
public:
    explicit GridSystem(graphics::GraphicsContext& graphicsContext);
    void update(entities::Registry& registry, core::Window& window);

private:
    graphics::Mesh createMesh(const components::Grid& grid);

    graphics::GraphicsContext& graphicsContext_;
    uint32_t last_mesh_handle_ = 0;
};

} // namespace systems
} // namespace exd
