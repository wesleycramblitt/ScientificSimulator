#pragma once

#include "entities/registry.hpp"

namespace exd {
namespace core {
class Window;
} // namespace core

namespace systems {

class ImGuizmoSystem;  // forward declaration

class ImGuiSystem {
public:
    ImGuiSystem();
    ~ImGuiSystem();

    ImGuiSystem(const ImGuiSystem&) = delete;
    ImGuiSystem& operator=(const ImGuiSystem&) = delete;

    // Must be called once after Window is ready (GL context created).
    // Returns false on failure.
    bool init(core::Window& window);

    // Must be called exactly once before shutdown.
    void shutdown();

    // Set the gizmo system reference (must be called before update).
    void setGizmoSystem(ImGuizmoSystem* gizmo) { gizmoSystem_ = gizmo; }

    // Call each frame. Handles ImGui event processing, frame begin,
    // panel drawing, gizmo rendering, frame end, and rendering.
    void update(entities::Registry& registry, const core::Window& window);

private:
    void drawEntityList(entities::Registry& registry);
    void drawViewportInfo(entities::Registry& registry, const core::Window& window);
    void drawComponentDetails(entities::Registry& registry);

    // Checks known component types for an entity, returns human-readable labels.
    static std::vector<const char*> componentTags(const entities::Registry& registry, entities::Entity e);

    bool initialized_ = false;

    entities::Entity    selected_entity_{};
    const char*         selected_tag_  = nullptr;

    ImGuizmoSystem*     gizmoSystem_ = nullptr;
};

} // namespace systems
} // namespace exd
