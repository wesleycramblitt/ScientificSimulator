#include "systems/imgui_system.hpp"
#include "systems/imguizmo_system.hpp"
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
#include "components/readonly.hpp"
#include "components/selected.hpp"
#include "components/skew.hpp"
#include "components/simulation_status.hpp"
#include "components/fluidx3d_config.hpp"
#include "components/fluid_domain.hpp"
#include "components/fluid_physics.hpp"
#include "components/particle_cloud.hpp"
#include "components/volume_field.hpp"
#include "components/simulation_reference.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

namespace exd {
namespace systems {

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

bool ImGuiSystem::init(core::Window& window) {
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

void ImGuiSystem::update(entities::Registry& registry, const core::Window& window) {
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

    // --- Gizmo rendering (must happen during ImGui frame, before Render) ---
    if (gizmoSystem_) {
        gizmoSystem_->renderGizmos(registry, window);
    }

    drawViewportInfo(registry, window);

    // --- End frame and render ---
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// -----------------------------------------------------------------------
// Entity list panel (docked: left)
// -----------------------------------------------------------------------

void ImGuiSystem::drawEntityList(entities::Registry& registry) {
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
        const bool isReadOnly = registry.has<components::ReadOnly>(e);

        // Entity-level disable toggle (hidden for read-only entities)
        bool entityEnabled = !registry.has<components::Disabled>(e);
        ImGui::PushID((int)e.id);
        if (!isReadOnly) {
            if (ImGui::Checkbox("##enabled", &entityEnabled)) {
                if (entityEnabled) registry.remove<components::Disabled>(e);
                else               registry.emplace<components::Disabled>(e);
            }
            ImGui::SameLine();
        }

        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow;
        if (tags.empty()) node_flags |= ImGuiTreeNodeFlags_Leaf;

        // Highlight selected entity
        bool isSelected = registry.has<components::Selected>(e);
        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 1.0f, 1.0f));
        }

        if (!entityEnabled)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));

        std::string entity_name = e.name + " Entity #" + std::to_string(e.id);
        bool open = ImGui::TreeNodeEx((void*)(intptr_t)e.id, node_flags, entity_name.c_str());

        if (!entityEnabled)
            ImGui::PopStyleColor();

        if (isSelected) {
            ImGui::PopStyleColor();
        }

        ImGui::PopID();

        if (open) {
            for (const auto* tag : tags) {
                ImGui::PushID(tag);
                if (ImGui::Selectable(tag, selected_tag_ == tag && selected_entity_ == e)) {
                    selected_entity_ = e;
                    selected_tag_    = tag;

                    // Set Selected tag on this entity, remove from all others
                    for (auto other : registry.view<components::Selected>()) {
                        if (other != e) registry.remove<components::Selected>(other);
                    }
                    if (!registry.has<components::Selected>(e)) {
                        registry.emplace<components::Selected>(e);
                    }
                }
                ImGui::PopID();
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

void ImGuiSystem::drawViewportInfo(entities::Registry& registry, const core::Window& window) {
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
    const bool isFPS = window.getInputMode() == common::InputMode::FPS;
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
    for (auto cam : registry.view<components::Camera, components::Transform>()) {
        auto& t = registry.get<components::Transform>(cam);
        ImGui::Text("Cam: %.1f, %.1f, %.1f", t.position.x, t.position.y, t.position.z);
        break;
    }

    // -- Gizmo mode (only shown when a gizmo system is active) --
    if (gizmoSystem_) {
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        int op = gizmoSystem_->currentOperation();
        int md = gizmoSystem_->currentMode();
        bool skewMode = gizmoSystem_->isSkewMode();

        // Clickable operation buttons
        auto opButton = [&](const char* label, int targetOp) {
            bool active = (!skewMode && op == targetOp);
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text,  ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            if (ImGui::SmallButton(label)) {
                gizmoSystem_->setOperation(targetOp);
            }
            if (active) {
                ImGui::PopStyleColor(2);
            }
        };

        opButton("T", 7);       // TRANSLATE
        ImGui::SameLine(0, 2);
        opButton("R", 120);     // ROTATE
        ImGui::SameLine(0, 2);
        opButton("S", 1792);    // SCALE
        ImGui::SameLine(0, 2);

        // Skew button
        {
            if (skewMode) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.8f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text,  ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            if (ImGui::SmallButton("K")) {
                gizmoSystem_->setSkewMode(registry);
            }
            if (skewMode) {
                ImGui::PopStyleColor(2);
            }
        }
        ImGui::SameLine();

        // Mode toggle button
        const char* mdStr = (md == 0) ? "Local" : "World";
        if (ImGui::SmallButton(mdStr)) {
            gizmoSystem_->toggleMode();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("[Tab]");

        // Show selection count + deselect button
        {
            int selCount = 0;
            for (auto _ : registry.view<components::Selected>()) { ++selCount; }
            if (selCount > 0) {
                ImGui::SameLine();
                ImGui::TextDisabled("|");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "%d selected", selCount);
                ImGui::SameLine();
                if (ImGui::SmallButton("X")) {
                    for (auto s : registry.view<components::Selected>()) {
                        registry.remove<components::Selected>(s);
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Deselect [Backspace]");
                }
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(1);
}

// -----------------------------------------------------------------------
// Component details popup
// -----------------------------------------------------------------------

void ImGuiSystem::drawComponentDetails(entities::Registry& registry) {
    if (!selected_tag_ || !registry.valid(selected_entity_)) return;

    const entities::Entity e = selected_entity_;
    const char* tag = selected_tag_;

    ImGui::SetNextWindowSize(ImVec2(340, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Component Details", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("%s — %s Entity #%u", tag, e.name.c_str(), e.id);
    ImGui::Separator();

    const bool isReadOnly = registry.has<components::ReadOnly>(e);

    // Disable interaction for read-only entities
    if (isReadOnly) ImGui::BeginDisabled(true);

    // ── Transform ──
    if (strcmp(tag, "Transform") == 0 && registry.has<components::Transform>(e)) {
        auto& t = registry.get<components::Transform>(e);
        ImGui::DragFloat3("Position", &t.position.x, 0.1f);
        ImGui::DragFloat4("Rotation", &t.rotation.w, 0.01f);
        ImGui::DragFloat3("Scale",    &t.scale.x, 0.01f);
    }
    // ── Camera ──
    else if (strcmp(tag, "Camera") == 0 && registry.has<components::Camera>(e)) {
        auto& c = registry.get<components::Camera>(e);
        float fov_deg = c.fov_y_radians * 180.0f / 3.1415926535f;
        if (ImGui::DragFloat("FOV (deg)", &fov_deg, 0.5f, 1.0f, 179.0f))
            c.fov_y_radians = fov_deg * 3.1415926535f / 180.0f;
        ImGui::DragFloat("Near",      &c.near_plane, 0.01f, 0.001f, 1000.0f);
        ImGui::DragFloat("Far",       &c.far_plane,  10.0f, 0.1f, 100000.0f);
        ImGui::DragFloat("Exposure",  &c.exposure,   0.01f, 0.0f, 100.0f);
    }
    // ── CameraController ──
    else if (strcmp(tag, "CameraController") == 0 && registry.has<components::CameraController>(e)) {
        auto& cc = registry.get<components::CameraController>(e);
        ImGui::DragFloat("Move speed",      &cc.move_speed,       0.5f,  0.1f, 500.0f);
        ImGui::DragFloat("Sprint mult",     &cc.sprint_mult,      0.1f,  1.0f, 10.0f);
        ImGui::DragFloat("Mouse sens",      &cc.mouse_sensitivity, 0.001f, 0.0f, 1.0f, "%.4f");
        ImGui::DragFloat("Yaw",             &cc.yaw,              0.5f);
        ImGui::DragFloat("Pitch",           &cc.pitch,            0.5f);
    }
    // ── Renderable ──
    else if (strcmp(tag, "Renderable") == 0 && registry.has<components::Renderable>(e)) {
        auto& r = registry.get<components::Renderable>(e);
        int mesh = (int)r.mesh;
        ImGui::InputInt("Mesh handle", &mesh, 0, 0, ImGuiInputTextFlags_ReadOnly);
    }
    // ── MeshAsset ──
    else if (strcmp(tag, "MeshAsset") == 0 && registry.has<components::MeshAsset>(e)) {
        auto& ma = registry.get<components::MeshAsset>(e);
        ImGui::Text("Path:  %s", ma.path.c_str());
    }
    // ── Cube ──
    else if (strcmp(tag, "Cube") == 0 && registry.has<components::Cube>(e)) {
        auto& c = registry.get<components::Cube>(e);
        ImGui::DragFloat("Size", &c.size, 0.01f, 0.01f, 1000.0f);
    }
    // ── Sphere ──
    else if (strcmp(tag, "Sphere") == 0 && registry.has<components::Sphere>(e)) {
        auto& s = registry.get<components::Sphere>(e);
        ImGui::DragFloat("Radius", &s.radius, 0.01f, 0.01f, 1000.0f);
    }
    // ── Box ──
    else if (strcmp(tag, "Box") == 0 && registry.has<components::Box>(e)) {
        auto& b = registry.get<components::Box>(e);
        ImGui::DragFloat3("Half extents", &b.halfExtents.x, 0.01f, 0.01f, 1000.0f);
    }
    // ── Capsule ──
    else if (strcmp(tag, "Capsule") == 0 && registry.has<components::Capsule>(e)) {
        auto& c = registry.get<components::Capsule>(e);
        ImGui::DragFloat("Radius",      &c.radius,     0.01f, 0.01f, 1000.0f);
        ImGui::DragFloat("Half height", &c.halfHeight, 0.01f, 0.01f, 1000.0f);
    }
    // ── Cylinder ──
    else if (strcmp(tag, "Cylinder") == 0 && registry.has<components::Cylinder>(e)) {
        auto& c = registry.get<components::Cylinder>(e);
        ImGui::DragFloat("Radius",      &c.radius,     0.01f, 0.01f, 1000.0f);
        ImGui::DragFloat("Half height", &c.halfHeight, 0.01f, 0.01f, 1000.0f);
    }
    // ── Plane ──
    else if (strcmp(tag, "Plane") == 0 && registry.has<components::Plane>(e)) {
        auto& p = registry.get<components::Plane>(e);
        ImGui::DragFloat("Width",  &p.width,  0.01f, 0.01f, 1000.0f);
        ImGui::DragFloat("Height", &p.height, 0.01f, 0.01f, 1000.0f);
    }
    // ── Grid ──
    else if (strcmp(tag, "Grid") == 0 && registry.has<components::Grid>(e)) {
        auto& g = registry.get<components::Grid>(e);
        ImGui::DragFloat("Spacing", &g.spacing, 0.1f, 0.1f, 1000.0f);
        ImGui::DragFloat3("Color", &g.color.x, 0.01f, 0.0f, 1.0f);
    }
    // ── Skew ──
    else if (strcmp(tag, "Skew") == 0 && registry.has<components::Skew>(e)) {
        auto& sk = registry.get<components::Skew>(e);
        ImGui::Text("Shear factors (T * R * K * S):");
        ImGui::DragFloat("XY shear", &sk.shear.x, 0.01f, -5.0f, 5.0f, "%.3f");
        ImGui::DragFloat("XZ shear", &sk.shear.y, 0.01f, -5.0f, 5.0f, "%.3f");
        ImGui::DragFloat("YZ shear", &sk.shear.z, 0.01f, -5.0f, 5.0f, "%.3f");
        if (ImGui::Button("Reset Skew")) {
            sk.shear = math::Vec3{0.0f, 0.0f, 0.0f};
        }
    }
    // ── CubeMap ──
    else if (strcmp(tag, "CubeMap") == 0 && registry.has<components::CubeMap>(e)) {
        auto& cm = registry.get<components::CubeMap>(e);
        ImGui::Text("Name:           %s", cm.name.c_str());
        ImGui::Text("Texture handle: %u", cm.texture_handle);
        ImGui::Text("Faces:          %zu", cm.faces.size());
    }
    // ── SimulationDomain ──
    else if (strcmp(tag, "SimulationDomain") == 0 && registry.has<components::SimulationDomain>(e)) {
        auto& d = registry.get<components::SimulationDomain>(e);
        ImGui::DragInt("NX", &d.nx, 1.0f, 1, 2048);
        ImGui::DragInt("NY", &d.ny, 1.0f, 1, 2048);
        ImGui::DragInt("NZ", &d.nz, 1.0f, 1, 2048);
    }
    // ── FluidPhysics ──
    else if (strcmp(tag, "FluidPhysics") == 0 && registry.has<components::FluidPhysics>(e)) {
        auto& p = registry.get<components::FluidPhysics>(e);
        ImGui::DragFloat("Viscosity",       &p.nu,                0.0001f, 0.0f, 1.0f, "%.4f");
        ImGui::DragFloat("Stream velocity", &p.streamwise_velocity, 0.001f, -1.0f, 1.0f, "%.3f");
        ImGui::DragInt("Stream axis",       (int*)&p.streamwise_axis, 1.0f, 0, 2);
        ImGui::DragFloat("Force X", &p.fx, 0.0001f, -1.0f, 1.0f, "%.4f");
        ImGui::DragFloat("Force Y", &p.fy, 0.0001f, -1.0f, 1.0f, "%.4f");
        ImGui::DragFloat("Force Z", &p.fz, 0.0001f, -1.0f, 1.0f, "%.4f");
        ImGui::DragFloat("Surf tension",   &p.sigma,             0.0001f, 0.0f, 1.0f, "%.4f");
    }
    // ── FluidX3DSolverConfig ──
    else if (strcmp(tag, "FluidX3DSolverConfig") == 0 && registry.has<components::FluidX3DSolverConfig>(e)) {
        auto& c = registry.get<components::FluidX3DSolverConfig>(e);
        ImGui::DragInt("Velocity set", (int*)&c.velocity_set, 1.0f, 0, 255);
        ImGui::DragInt("Collision",    (int*)&c.collision,    1.0f, 0, 255);
        ImGui::DragInt("Precision",    (int*)&c.precision,    1.0f, 0, 255);
        ImGui::DragInt("DX", (int*)&c.dx, 1.0f, 1, 128);
        ImGui::DragInt("DY", (int*)&c.dy, 1.0f, 1, 128);
        ImGui::DragInt("DZ", (int*)&c.dz, 1.0f, 1, 128);
        ImGui::Text("Extensions:    0x%X", c.extensions);
    }
    // ── SimulationInfo ──
    else if (strcmp(tag, "SimulationInfo") == 0 && registry.has<components::SimulationInfo>(e)) {
        auto& s = registry.get<components::SimulationInfo>(e);
        const char* status_str = s.status == components::SimulationStatus::Running  ? "Running"
                               : s.status == components::SimulationStatus::Stopped   ? "Stopped"
                               : s.status == components::SimulationStatus::Completed ? "Completed"
                               : "Error";
        ImGui::Text("Status:      %s", status_str);
        ImGui::Text("Step:        %u / %u", s.current_step, s.total_steps);
        ImGui::Text("Target:      900 steps/s (time-synced)");
        int spf = (int)s.steps_per_frame;
        if (ImGui::DragInt("Max s/frame", &spf, 1.0f, 1, 1000))
            s.steps_per_frame = (uint32_t)spf;
    }
    // ── SimulationReference ──
    else if (strcmp(tag, "SimulationReference") == 0 && registry.has<components::SimulationReference>(e)) {
        auto& sr = registry.get<components::SimulationReference>(e);
        int sim_id = (int)sr.simulation_entity_id;
        ImGui::InputInt("Sim Entity ID", &sim_id, 0, 0, ImGuiInputTextFlags_ReadOnly);
    }
    // ── ParticleCloud ──
    else if (strcmp(tag, "ParticleCloud") == 0 && registry.has<components::ParticleCloud>(e)) {
        auto& pc = registry.get<components::ParticleCloud>(e);
        ImGui::Text("Particles:  %d / %d", pc.particle_count, pc.max_particles);
        int mp = pc.max_particles;
        if (ImGui::DragInt("Max Particles", &mp, 1000.0f, 0, 5000000))
            pc.max_particles = mp;
    }
    // ── VolumeField ──
    else if (strcmp(tag, "VolumeField") == 0 && registry.has<components::VolumeField>(e)) {
        auto& vf = registry.get<components::VolumeField>(e);
        ImGui::Text("Texture:    %u", vf.texture_handle);
        ImGui::Text("Ready:      %s", vf.interop_ready ? "yes" : "no");
    }

    if (isReadOnly) ImGui::EndDisabled();

    // Close button
    if (ImGui::Button("Close")) {
        selected_tag_ = nullptr;
    }

    ImGui::End();
}

// -----------------------------------------------------------------------
// Component introspection
// -----------------------------------------------------------------------

std::vector<const char*> ImGuiSystem::componentTags(const entities::Registry& registry, entities::Entity e) {
    std::vector<const char*> tags;

    // Use try_get (not has) so disabled components still appear in the tree.
    // has<T>() returns false when enabled==false, which would make them invisible
    // with no way to re-enable.  Downstream systems still filter via has<T>().
    if (registry.try_get<components::Transform>(e))         tags.push_back("Transform");
    if (registry.try_get<components::Camera>(e))            tags.push_back("Camera");
    if (registry.try_get<components::CameraController>(e))  tags.push_back("CameraController");
    if (registry.try_get<components::Renderable>(e))        tags.push_back("Renderable");
    if (registry.try_get<components::CubeMap>(e))           tags.push_back("CubeMap");
    if (registry.try_get<components::MeshAsset>(e))         tags.push_back("MeshAsset");
    if (registry.try_get<components::Cube>(e))              tags.push_back("Cube");
    if (registry.try_get<components::Sphere>(e))            tags.push_back("Sphere");
    if (registry.try_get<components::Box>(e))               tags.push_back("Box");
    if (registry.try_get<components::Capsule>(e))           tags.push_back("Capsule");
    if (registry.try_get<components::Cylinder>(e))          tags.push_back("Cylinder");
    if (registry.try_get<components::Plane>(e))             tags.push_back("Plane");
    if (registry.try_get<components::Grid>(e))              tags.push_back("Grid");
    if (registry.try_get<components::Selected>(e))           tags.push_back("Selected");
    if (registry.try_get<components::Skew>(e))               tags.push_back("Skew");
    if (registry.try_get<components::SimulationDomain>(e))       tags.push_back("SimulationDomain");
    if (registry.try_get<components::FluidPhysics>(e))           tags.push_back("FluidPhysics");
    if (registry.try_get<components::FluidX3DSolverConfig>(e))   tags.push_back("FluidX3DSolverConfig");
    if (registry.try_get<components::SimulationInfo>(e))         tags.push_back("SimulationInfo");
    if (registry.try_get<components::SimulationReference>(e))    tags.push_back("SimulationReference");
    if (registry.try_get<components::ParticleCloud>(e))          tags.push_back("ParticleCloud");
    if (registry.try_get<components::VolumeField>(e))            tags.push_back("VolumeField");
    if (registry.try_get<components::Disabled>(e))               tags.push_back("Disabled");

    return tags;
}

} // namespace systems
} // namespace exd
