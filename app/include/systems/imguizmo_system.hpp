#pragma once
#include "entities/registry.hpp"
#include "math/mat4.hpp"
#include "math/vec3.hpp"
#include "math/quat.hpp"

namespace exd {
namespace graphics {
class GraphicsContext;
} // namespace graphics
namespace core {
class Window;
} // namespace core

namespace systems {

/// Manages ImGuizmo transform gizmos (Translate, Rotate, Scale) and
/// optionally a view-manipulation corner gizmo.
///
/// Lifecycle:
///   - update()      — called once per frame (before ImGui frame) to handle
///                     keyboard shortcuts for operation/mode switching.
///   - renderGizmos() — called *during* the ImGui frame (between NewFrame
///                       and Render) to draw manipulate gizmos over the
///                       selected entity.
class ImGuizmoSystem {
public:
    ImGuizmoSystem(graphics::GraphicsContext& graphicsContext);
    ~ImGuizmoSystem();

    ImGuizmoSystem(const ImGuizmoSystem&) = delete;
    ImGuizmoSystem& operator=(const ImGuizmoSystem&) = delete;

    /// Handle keyboard shortcuts for gizmo mode/operation.
    /// Safe to call before or during the ImGui frame (does not use ImGui).
    void update(entities::Registry& registry, const core::Window& window);

    /// Draw transform gizmos for the selected entity.
    /// MUST be called between ImGui::NewFrame() and ImGui::Render().
    void renderGizmos(entities::Registry& registry, const core::Window& window);

    /// --- accessors for ImGui panels ---
    int  currentOperation() const { return currentOp_; }
    int  currentMode() const      { return currentMode_; }
    bool isSkewMode() const       { return skewMode_; }
    void setOperation(int op)     { currentOp_ = op; skewMode_ = false; }
    void setSkewMode(entities::Registry& registry);  // activates skew, adds Skew component if needed
    void toggleMode()             { currentMode_ = (currentMode_ == 0) ? 1 : 0; }
    bool snapEnabled() const      { return snapEnabled_; }
    const float* snapValues() const { return snapValues_; }

private:
    // Current gizmo operation and coordinate mode
    // Values are ImGuizmo::OPERATION and ImGuizmo::MODE, initialized in .cpp
    int currentOp_   = 7;  // ImGuizmo::TRANSLATE
    int currentMode_ = 0;  // ImGuizmo::MODE::LOCAL
    bool skewMode_   = false;  // true → skew editing active (no ImGuizmo gizmo)

    bool snapEnabled_ = false;
    float snapValues_[3] = {1.0f, 1.0f, 1.0f};

    // Cached object matrix for continuity across frames (ImGuizmo modifies in-place)
    float objectMatrix_[16];
    bool  matrixValid_ = false;

    /// Extract camera view and projection matrices from the registry.
    bool getCameraMatrices(const entities::Registry& registry,
                           math::Mat4& view, math::Mat4& proj,
                           float aspect) const;

    /// Decompose a 4x4 column-major model matrix into position, rotation, scale.
    /// Rotation is extracted from the upper-left 3x3 after removing scale.
    void decomposeMatrix(const float* matrix,
                         math::Vec3& pos, math::Quat& rot, math::Vec3& scale) const;

    /// Recompute the object matrix from an entity's Transform + optional Skew.
    void buildObjectMatrix(const entities::Registry& registry, entities::Entity e,
                           float* outMatrix) const;

    /// Apply decomposed matrix back to the entity's Transform component.
    void applyToTransform(entities::Registry& registry, entities::Entity e,
                          const math::Vec3& pos, const math::Quat& rot, const math::Vec3& scale) const;

    /// Raycast from camera through mouse into the scene.  Selects the closest
    /// entity with a Transform + Renderable component that is hit.
    void pickEntity(entities::Registry& registry, const core::Window& window) const;

    graphics::GraphicsContext& ctx_;
};

} // namespace systems
} // namespace exd
