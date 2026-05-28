#pragma once

#include "entities/registry.hpp"

class Window;

class ImGuiSystem {
public:
    ImGuiSystem();
    ~ImGuiSystem();

    ImGuiSystem(const ImGuiSystem&) = delete;
    ImGuiSystem& operator=(const ImGuiSystem&) = delete;

    // Must be called once after Window is ready (GL context created).
    // Returns false on failure.
    bool init(Window& window);

    // Must be called exactly once before shutdown.
    void shutdown();

    // Call each frame. Handles ImGui event processing, frame begin,
    // panel drawing, frame end, and rendering.
    void update(Registry& registry, const Window& window);

private:
    void drawEntityList(Registry& registry);
    void drawViewportInfo(const Registry& registry, const Window& window);
    void drawComponentDetails(const Registry& registry);

    // Checks known component types for an entity, returns human-readable labels.
    static std::vector<const char*> componentTags(const Registry& registry, Entity e);

    bool initialized_ = false;

    Entity    selected_entity_{};
    const char* selected_tag_  = nullptr;
};

