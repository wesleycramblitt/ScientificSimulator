#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "math/mat4.hpp"
#include "math/vec3.hpp"

class GizmoSystem {
public:
    GizmoSystem();
    void update(Registry& registry, const Window& window);

private:
    void renderAxes() const;
    void handleClick(Registry& registry, const Window& window);

    // Gizmo viewport in pixels (bottom-right corner)
    int gx_, gy_, gw_, gh_;

    // Gizmo camera
    Mat4 gizmoView_, gizmoProj_;

    // Static VAO for the 3 axis lines
    unsigned vao_ = 0, vbo_ = 0;

    // Click state
    bool dragging_ = false;
};
