#include "systems/imgui_system.hpp"
#include "core/window.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

// All known component types for entity list introspection
#include "components/transform.hpp"
#include "components/camera.hpp"
#include "components/camera_controller.hpp"
#include "components/renderable.hpp"
#include "components/cubemap.hpp"
#include "components/mesh_asset.hpp"
#include "components/cube.hpp"
#include "components/sphere.hpp"
#include "components/box.hpp"
#include "components/capsule.hpp"
#include "components/cylinder.hpp"
#include "components/plane.hpp"
#include "components/grid.hpp"
#include "components/collider.hpp"

#include <cstdio>
#include <vector>

// -----------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------

ImGuiSystem::ImGuiSystem() = default;

ImGuiSystem::~ImGuiSystem() {
    if (initialized_) shutdown();
}

// -----------------------------------------------------------------------
// Init / Shutdown
// -----------------------------------------------------------------------

bool ImGuiSystem::init(Window& window) {
    if (initialized_) {
        std::fprintf(stderr, "ImGuiSystem already initialized.\n");
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL3_InitForOpenGL(window.window, window.context)) {
        std::fprintf(stderr, "ImGui_ImplSDL3_InitForOpenGL failed.\n");
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        std::fprintf(stderr, "ImGui_ImplOpenGL3_Init failed.\n");
        return false;
    }

    initialized_ = true;
    return true;
}

void ImGuiSystem::shutdown() {
    if (!initialized_) return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    initialized_ = false;
}

// -----------------------------------------------------------------------
// Per-frame update
// -----------------------------------------------------------------------

void ImGuiSystem::update(Registry& registry, const Window& window) {
    if (!initialized_) return;

    // --- Forward SDL events to ImGui ---
    for (const auto& event : window.event_buffer) {
        ImGui_ImplSDL3_ProcessEvent(&event);
    }

    // --- Begin ImGui frame ---
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // --- Panels ---
    drawEntityList(registry);
    drawViewportInfo(window);

    // --- End frame and render ---
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// -----------------------------------------------------------------------
// Entity list panel (docked: left)
// -----------------------------------------------------------------------

void ImGuiSystem::drawEntityList(const Registry& registry) {
    ImGuiViewport* vp = ImGui::GetMainViewport();

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 120));
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x * 0.20f, 0), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoDocking;

    ImGui::Begin("Entities", nullptr, flags);

    const auto entities = registry.all_entities();
    ImGui::Text("Entities: %zu", entities.size());
    ImGui::Separator();

    for (const auto& e : entities) {
        if (!registry.valid(e)) continue;

        const auto tags = componentTags(registry, e);

        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow;
        if (tags.empty()) node_flags |= ImGuiTreeNodeFlags_Leaf;

        std::string entity_name = e.name + " Entity #" + std::to_string(e.id);
        bool open = ImGui::TreeNodeEx((void*)(intptr_t)e.id, node_flags, entity_name.c_str());
        if (open) {
            for (const auto* tag : tags) {
                ImGui::BulletText("%s", tag);
            }
            ImGui::TreePop();
        }
    }

    ImGui::End();
    ImGui::PopStyleColor(1);
}

// -----------------------------------------------------------------------
// Viewport info overlay (floating, transparent, pinned top-right)
// -----------------------------------------------------------------------

void ImGuiSystem::drawViewportInfo(const Window& window) {
    // Semi-transparent dark background so text is readable over the 3D scene
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 120));

    ImGuiWindowFlags info_flags =
        ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoDocking
        | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoNavFocus
        | ImGuiWindowFlags_AlwaysAutoResize;

    // Bottom-right corner, above the future bottom panel (20% of viewport)
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float x = vp->WorkPos.x ;
    float y = vp->WorkPos.y + vp->WorkSize.y - 20.0f; // above the 20% bottom area + padding
    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,  ImVec2(14, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::Begin("Viewport Info", nullptr, info_flags);

    // -- FPS / UI mode --
    const bool isFPS = window.getInputMode() == InputMode::FPS;
    ImGui::TextColored(isFPS ? ImVec4(0.4f, 0.9f, 0.4f, 1.0f)
                             : ImVec4(0.9f, 0.7f, 0.2f, 1.0f),
                       "%s", isFPS ? "FPS Mode [F1]" : "UI Mode [F1]");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("F1 — toggle between FPS (camera control) and UI (editor interaction)");
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // -- Wireframe --
    if (window.wireframe) {
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Wireframe Mode: Toggle Z");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Fill Mode [X]");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("X — wireframe  |  Z — fill");
    }

    ImGui::End();
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(1);
}

// -----------------------------------------------------------------------
// Component introspection
// -----------------------------------------------------------------------

std::vector<const char*> ImGuiSystem::componentTags(const Registry& registry, Entity e) {
    std::vector<const char*> tags;

    if (registry.has<Transform>(e))         tags.push_back("Transform");
    if (registry.has<Camera>(e))            tags.push_back("Camera");
    if (registry.has<CameraController>(e))  tags.push_back("CameraController");
    if (registry.has<Renderable>(e))        tags.push_back("Renderable");
    if (registry.has<CubeMap>(e))           tags.push_back("CubeMap");
    if (registry.has<MeshAsset>(e))         tags.push_back("MeshAsset");
    if (registry.has<Cube>(e))              tags.push_back("Cube");
    if (registry.has<Sphere>(e))            tags.push_back("Sphere");
    if (registry.has<Box>(e))               tags.push_back("Box");
    if (registry.has<Capsule>(e))           tags.push_back("Capsule");
    if (registry.has<Cylinder>(e))          tags.push_back("Cylinder");
    if (registry.has<Plane>(e))             tags.push_back("Plane");
    if (registry.has<Grid>(e))              tags.push_back("Grid");
    if (registry.has<Collider>(e))          tags.push_back("Collider");

    return tags;
}
