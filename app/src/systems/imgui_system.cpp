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
#include "components/disabled.hpp"
#include "components/simulation_status.hpp"
#include "components/fluidx3d_config.hpp"
#include "components/fluid_domain.hpp"
#include "components/fluid_physics.hpp"

#include <cstdio>
#include <cstring>
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
    drawComponentDetails(registry);
    drawViewportInfo(registry, window);

    // --- End frame and render ---
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// -----------------------------------------------------------------------
// Entity list panel (docked: left)
// -----------------------------------------------------------------------

void ImGuiSystem::drawEntityList(Registry& registry) {
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
        const bool isCamera = registry.has<Camera>(e);
        const bool isGrid   = registry.has<Grid>(e);

        // Disable toggle (Camera entities are immune)
        bool enabled = !registry.has<Disabled>(e);
        ImGui::PushID((int)e.id);
        if (!isCamera && !isGrid) {
            if (ImGui::Checkbox("##enabled", &enabled)) {
                if (enabled) registry.remove<Disabled>(e);
                else         registry.emplace<Disabled>(e);
            }
            ImGui::SameLine();
        }

        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow;
        if (tags.empty()) node_flags |= ImGuiTreeNodeFlags_Leaf;

        if (!enabled)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));

        std::string entity_name = e.name + " Entity #" + std::to_string(e.id);
        bool open = ImGui::TreeNodeEx((void*)(intptr_t)e.id, node_flags, entity_name.c_str());

        if (!enabled)
            ImGui::PopStyleColor();

        ImGui::PopID();

        if (open) {
            for (const auto* tag : tags) {
                if (ImGui::Selectable(tag, selected_tag_ == tag && selected_entity_ == e)) {
                    selected_entity_ = e;
                    selected_tag_    = tag;
                }
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

void ImGuiSystem::drawViewportInfo(const Registry& registry, const Window& window) {
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
                       "%s", isFPS ? "FPS Mode [Z]" : "UI Mode [Z]");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Z — toggle between FPS (camera control) and UI (editor interaction)");
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // -- Wireframe --
    if (window.wireframe) {
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Wireframe [X]");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.6f, 1.0f), "Fill [X]");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("X - toggle between Wireframe and Fill Modes");
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // -- Grid --
    if (window.grid_visible) {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Grid [G]");
    } else {
        ImGui::TextDisabled("Grid [G]");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("G - toggle grid");
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // -- Simulation --
    if (window.simulation_mode) {
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.9f, 1.0f), "Sim [T]");
    } else {
        ImGui::TextDisabled("Sim [T]");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("T - toggle simulation");
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // -- Camera position --
    for (auto cam : registry.view<Camera, Transform>()) {
        auto& t = registry.get<Transform>(cam);
        ImGui::Text("Cam: %.1f, %.1f, %.1f", t.position.x, t.position.y, t.position.z);
        break;
    }

    ImGui::End();
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(1);
}

// -----------------------------------------------------------------------
// Component details popup
// -----------------------------------------------------------------------

void ImGuiSystem::drawComponentDetails(const Registry& registry) {
    if (!selected_tag_ || !registry.valid(selected_entity_)) return;

    const Entity e = selected_entity_;
    const char* tag = selected_tag_;

    ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Component Details", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("%s — %s Entity #%u", tag, e.name.c_str(), e.id);
    ImGui::Separator();

    // Transform
    if (strcmp(tag, "Transform") == 0 && registry.has<Transform>(e)) {
        auto& t = registry.get<Transform>(e);
        ImGui::Text("Position:  %.2f, %.2f, %.2f", t.position.x, t.position.y, t.position.z);
        ImGui::Text("Rotation:  %.3f, %.3f, %.3f, %.3f", t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w);
        ImGui::Text("Scale:     %.2f, %.2f, %.2f", t.scale.x, t.scale.y, t.scale.z);
    }
    // Camera
    else if (strcmp(tag, "Camera") == 0 && registry.has<Camera>(e)) {
        auto& c = registry.get<Camera>(e);
        ImGui::Text("FOV:       %.2f", c.fov_y_radians);
        ImGui::Text("Near:      %.3f", c.near_plane);
        ImGui::Text("Far:       %.1f", c.far_plane);
        ImGui::Text("Exposure:  %.2f", c.exposure);
    }
    // CameraController
    else if (strcmp(tag, "CameraController") == 0 && registry.has<CameraController>(e)) {
        auto& cc = registry.get<CameraController>(e);
        ImGui::Text("Move speed:     %.2f", cc.move_speed);
        ImGui::Text("Sprint mult:    %.2f", cc.sprint_mult);
        ImGui::Text("Mouse sens:     %.4f", cc.mouse_sensitivity);
        ImGui::Text("Yaw:            %.2f", cc.yaw);
        ImGui::Text("Pitch:          %.2f", cc.pitch);
    }
    // Renderable
    else if (strcmp(tag, "Renderable") == 0 && registry.has<Renderable>(e)) {
        auto& r = registry.get<Renderable>(e);
        ImGui::Text("Mesh handle:  %u", r.mesh);
    }
    // MeshAsset
    else if (strcmp(tag, "MeshAsset") == 0 && registry.has<MeshAsset>(e)) {
        auto& ma = registry.get<MeshAsset>(e);
        ImGui::Text("Path:  %s", ma.path.c_str());
    }
    // Cube
    else if (strcmp(tag, "Cube") == 0 && registry.has<Cube>(e)) {
        ImGui::Text("Size:  %.2f", registry.get<Cube>(e).size);
    }
    // Sphere
    else if (strcmp(tag, "Sphere") == 0 && registry.has<Sphere>(e)) {
        ImGui::Text("Radius:  %.2f", registry.get<Sphere>(e).radius);
    }
    // Box
    else if (strcmp(tag, "Box") == 0 && registry.has<Box>(e)) {
        auto& b = registry.get<Box>(e);
        ImGui::Text("Half extents:  %.2f, %.2f, %.2f", b.halfExtents.x, b.halfExtents.y, b.halfExtents.z);
    }
    // Capsule
    else if (strcmp(tag, "Capsule") == 0 && registry.has<Capsule>(e)) {
        auto& c = registry.get<Capsule>(e);
        ImGui::Text("Radius:      %.2f", c.radius);
        ImGui::Text("Half height: %.2f", c.halfHeight);
    }
    // Cylinder
    else if (strcmp(tag, "Cylinder") == 0 && registry.has<Cylinder>(e)) {
        auto& c = registry.get<Cylinder>(e);
        ImGui::Text("Radius:      %.2f", c.radius);
        ImGui::Text("Half height: %.2f", c.halfHeight);
    }
    // Plane
    else if (strcmp(tag, "Plane") == 0 && registry.has<Plane>(e)) {
        auto& p = registry.get<Plane>(e);
        ImGui::Text("Width:   %.2f", p.width);
        ImGui::Text("Height:  %.2f", p.height);
    }
    // Grid
    else if (strcmp(tag, "Grid") == 0 && registry.has<Grid>(e)) {
        auto& g = registry.get<Grid>(e);
        ImGui::Text("Spacing:    %.1f", g.spacing);
        ImGui::Text("Color:      %.2f, %.2f, %.2f", g.color.x, g.color.y, g.color.z);
    }
    // CubeMap
    else if (strcmp(tag, "CubeMap") == 0 && registry.has<CubeMap>(e)) {
        auto& cm = registry.get<CubeMap>(e);
        ImGui::Text("Name:           %s", cm.name.c_str());
        ImGui::Text("Texture handle: %u", cm.texture_handle);
        ImGui::Text("Faces:          %zu", cm.faces.size());
    }
    // SimulationDomain
    else if (strcmp(tag, "SimulationDomain") == 0 && registry.has<SimulationDomain>(e)) {
        auto& d = registry.get<SimulationDomain>(e);
        ImGui::Text("Grid:  %d x %d x %d", d.nx, d.ny, d.nz);
    }
    // FluidPhysics
    else if (strcmp(tag, "FluidPhysics") == 0 && registry.has<FluidPhysics>(e)) {
        auto& p = registry.get<FluidPhysics>(e);
        ImGui::Text("Viscosity:        %.4f", p.nu);
        ImGui::Text("Stream velocity:  %.3f", p.streamwise_velocity);
        ImGui::Text("Stream axis:      %u",   p.streamwise_axis);
        ImGui::Text("Force:            %.4f, %.4f, %.4f", p.fx, p.fy, p.fz);
        ImGui::Text("Surface tension:  %.4f", p.sigma);
    }
    // FluidX3DSolverConfig
    else if (strcmp(tag, "FluidX3DSolverConfig") == 0 && registry.has<FluidX3DSolverConfig>(e)) {
        auto& c = registry.get<FluidX3DSolverConfig>(e);
        ImGui::Text("Velocity set:  %u", c.velocity_set);
        ImGui::Text("Collision:     %u", c.collision);
        ImGui::Text("Precision:     %u", c.precision);
        ImGui::Text("Subdivisions:  %u x %u x %u", c.dx, c.dy, c.dz);
        ImGui::Text("Extensions:    0x%X", c.extensions);
    }
    // SimulationInfo
    else if (strcmp(tag, "SimulationInfo") == 0 && registry.has<SimulationInfo>(e)) {
        auto& s = registry.get<SimulationInfo>(e);
        ImGui::Text("Status:      %s", s.status == Running ? "Running" : s.status == Stopped ? "Stopped" : "Error");
        ImGui::Text("Step:        %u / %u", s.current_step, s.total_steps);
        ImGui::Text("S/frame:     %u", s.steps_per_frame);
    }

    // Close button
    if (ImGui::Button("Close")) {
        selected_tag_ = nullptr;
    }

    ImGui::End();
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
    if (registry.has<SimulationDomain>(e))       tags.push_back("SimulationDomain");
    if (registry.has<FluidPhysics>(e))           tags.push_back("FluidPhysics");
    if (registry.has<FluidX3DSolverConfig>(e))   tags.push_back("FluidX3DSolverConfig");
    if (registry.has<SimulationInfo>(e))         tags.push_back("SimulationInfo");
    if (registry.has<Disabled>(e))               tags.push_back("Disabled");

    return tags;
}
