#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "math/mat4.hpp"
#include "math/vec3.hpp"

namespace exd {
namespace graphics {
class GraphicsContext;
} // namespace graphics

namespace systems {

class GizmoSystem {
public:
    GizmoSystem(graphics::GraphicsContext& graphicsContext);
    void update(entities::Registry& registry, const core::Window& window);

private:
    void renderAxes() const;
    void handleClick(entities::Registry& registry, const core::Window& window);

    // Gizmo viewport in pixels (bottom-right corner)
    int gx_, gy_, gw_, gh_;

    // Gizmo camera
    math::Mat4 gizmoView_, gizmoProj_;

    // Static VAO for the 3 axis lines
    unsigned vao_ = 0, vbo_ = 0;

    // Click state
    bool dragging_ = false;

    graphics::GraphicsContext& graphicsContext_;
};

} // namespace systems
} // namespace exd
