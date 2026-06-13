#include "systems/imguizmo_system.hpp"
#include "core/window.hpp"
#include "components/transform.hpp"
#include "components/skew.hpp"
#include "components/selected.hpp"
#include "components/renderable.hpp"
#include "components/camera.hpp"
#include "components/camera_controller.hpp"
#include "components/disabled.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/mesh.hpp"
#include "graphics/vertex.hpp"

#include "imgui.h"       // must be before ImGuizmo.h
#include "ImGuizmo.h"

#include <cmath>
#include <cstring>
#include <algorithm>

namespace exd {
namespace systems {

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

ImGuizmoSystem::ImGuizmoSystem(graphics::GraphicsContext& graphicsContext)
    : ctx_(graphicsContext)
{
    // Initialise object matrix to identity
    std::memcpy(objectMatrix_, math::Mat4::identity().m, sizeof(objectMatrix_));
}

ImGuizmoSystem::~ImGuizmoSystem() = default;

// ---------------------------------------------------------------------------
// update — keyboard shortcuts (safe to call at any time)
// ---------------------------------------------------------------------------

void ImGuizmoSystem::update(entities::Registry& registry, const core::Window& window) {
    // Only process shortcuts when in UI mode (so we don't steal camera keys)
    if (window.getInputMode() != exd::common::InputMode::UI) return;

    // Don't steal keyboard input when ImGui text widgets are focused
    const bool imguiWantsKb = ImGui::GetIO().WantCaptureKeyboard;

    for (const auto& ev : window.event_buffer) {
        // --- Keyboard shortcuts ---
        if (ev.type == SDL_EVENT_KEY_DOWN && !imguiWantsKb) {
            switch (ev.key.scancode) {
                case SDL_SCANCODE_W:
                    currentOp_ = ImGuizmo::TRANSLATE;
                    skewMode_ = false;
                    break;
                case SDL_SCANCODE_E:
                    currentOp_ = ImGuizmo::ROTATE;
                    skewMode_ = false;
                    break;
                case SDL_SCANCODE_R:
                    currentOp_ = ImGuizmo::SCALE;
                    skewMode_ = false;
                    break;
                case SDL_SCANCODE_K:
                    setSkewMode(registry);
                    break;
                case SDL_SCANCODE_TAB:
                    currentMode_ = (currentMode_ == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                    break;
                case SDL_SCANCODE_BACKSPACE:
                    for (auto sel : registry.view<components::Selected>()) {
                        registry.remove<components::Selected>(sel);
                    }
                    break;
                default:
                    break;
            }
        }

        // --- Viewport click → raycast pick (left mouse only) ---
        if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            ev.button.button == SDL_BUTTON_LEFT &&
            !ImGui::GetIO().WantCaptureMouse &&       // not over an ImGui panel
            !ImGuizmo::IsOver() &&                    // not over a gizmo handle
            !ImGuizmo::IsUsing()) {                   // not dragging a gizmo

            pickEntity(registry, window);
        }
    }
}

// ---------------------------------------------------------------------------
// renderGizmos — draw ImGuizmo controls (MUST be called during ImGui frame)
// ---------------------------------------------------------------------------

void ImGuizmoSystem::renderGizmos(entities::Registry& registry, const core::Window& window) {
    // Don't draw gizmos in FPS mode — they interfere with camera control
    if (window.getInputMode() != exd::common::InputMode::UI) return;
    // --- Find selected entity ---
    entities::Entity selectedEntity{};
    for (auto e : registry.view<components::Selected>()) {
        selectedEntity = e;
        break;  // first selected entity only
    }

    // --- Get view/projection ---
    int vpW, vpH; float aspect;
    window.getDimensions(vpW, vpH, aspect);

    math::Mat4 view, proj;
    if (!getCameraMatrices(registry, view, proj, aspect)) return;

    // --- Set up ImGuizmo viewport ---
    ImGuizmo::BeginFrame();
    ImGuizmo::SetRect(0, 0, (float)vpW, (float)vpH);
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::Enable(true);

    // --- View-manipulate gizmo (corner widget) ---
    // Re-use the camera's view matrix; ImGuizmo::ViewManipulate modifies it in-place.
    {
        float viewCopy[16];
        std::memcpy(viewCopy, view.m, sizeof(viewCopy));

        // Draw in bottom-right corner
        const float vmSize = 128.0f;
        ImGuizmo::ViewManipulate(
            viewCopy,            // view matrix (modified in-place by the gizmo)
            8.0f,                // camera distance from target
            ImVec2((float)vpW - vmSize - 10.0f, 10.0f),  // position
            ImVec2(vmSize, vmSize),                        // size
            0x10101010           // background (low alpha)
        );

        // If the user rotated the view gizmo, apply the inverse to the camera entity
        // We detect a change by comparing the matrices.
        bool viewChanged = false;
        for (int i = 0; i < 16; ++i) {
            if (std::fabs(viewCopy[i] - view.m[i]) > 1e-6f) { viewChanged = true; break; }
        }

        if (viewChanged) {
            // ViewManipulate returns a view matrix looking at origin from distance.
            // V = lookAt(eye, origin, up) stored column-major:
            //   col0 = right, col1 = up, col2 = -forward, col3 = -R*eye
            // Therefore: eye = -(right, up, -forward)^T * col3
            // Using bwd = -forward (i.e. col2 unnegated):
            //   eye = -(right.col3.x + up.col3.y + bwd.col3.z)

            math::Vec3 right(viewCopy[0], viewCopy[1], viewCopy[2]);
            math::Vec3 up   (viewCopy[4], viewCopy[5], viewCopy[6]);
            math::Vec3 bwd  (viewCopy[8], viewCopy[9], viewCopy[10]);   // = -forward
            math::Vec3 t    (viewCopy[12], viewCopy[13], viewCopy[14]); // col3

            math::Vec3 eye(
                -(right.x*t.x + right.y*t.y + right.z*t.z),
                -(up.x*t.x    + up.y*t.y    + up.z*t.z),
                -(bwd.x*t.x   + bwd.y*t.y   + bwd.z*t.z)
            );

            // Apply to the camera entity
            for (auto camE : registry.view<components::Camera, components::CameraController, components::Transform>()) {
                auto& tf = registry.get<components::Transform>(camE);
                auto& cc = registry.get<components::CameraController>(camE);

                tf.position = eye;

                // Recompute yaw/pitch from the new forward vector
                math::Vec3 forward = math::Vec3{-viewCopy[8], -viewCopy[9], -viewCopy[10]};
                cc.yaw   = std::atan2(forward.x, -forward.z);
                cc.pitch = std::asin(forward.y);
                break;
            }
        }
    }

    // --- Transform gizmo for selected entity ---
    if (registry.valid(selectedEntity) && registry.has<components::Transform>(selectedEntity)) {
        // Build or refresh object matrix
        buildObjectMatrix(registry, selectedEntity, objectMatrix_);
        matrixValid_ = true;

        if (skewMode_) {
            // ── Skew mode: render translate handles, map delta to shear factors ──
            //
            // We run Manipulate with TRANSLATE so the user sees familiar arrows.
            // Instead of moving the object, we read the position delta and apply
            // it to the Skew component:  X → XY shear, Y → XZ shear, Z → YZ shear.
            auto& xform = registry.get<components::Transform>(selectedEntity);
            auto& sk    = registry.get<components::Skew>(selectedEntity);

            math::Vec3 origPos = xform.position;
            math::Vec3 origShear = sk.shear;

            // Build a TRS-only matrix (no skew) so Manipulate doesn't double-apply
            math::Mat4 trsOnly = math::Mat4::modelTRS(xform.position, xform.rotation, xform.scale);
            float skewMatrix[16];
            std::memcpy(skewMatrix, trsOnly.m, sizeof(skewMatrix));

            float deltaMatrix[16];
            if (ImGuizmo::Manipulate(
                    view.m, proj.m,
                    ImGuizmo::TRANSLATE,        // always show translate arrows
                    ImGuizmo::LOCAL,            // local-space skew feels more natural
                    skewMatrix,
                    deltaMatrix,
                    nullptr)) {

                // Extract the "fake" position after the user dragged
                math::Vec3 newPos{skewMatrix[12], skewMatrix[13], skewMatrix[14]};
                math::Vec3 delta = {newPos.x - origPos.x,
                                    newPos.y - origPos.y,
                                    newPos.z - origPos.z};

                // Sensitivity: 1 unit of translation = 0.5 shear factor
                const float sens = 0.5f;
                sk.shear.x += delta.x * sens;   // XY shear
                sk.shear.y += delta.y * sens;   // XZ shear
                sk.shear.z += delta.z * sens;   // YZ shear

                // Restore original position — the object doesn't move
                xform.position = origPos;
            }
        } else {
            // ── Normal mode: translate / rotate / scale via ImGuizmo ──
            float deltaMatrix[16];
            if (ImGuizmo::Manipulate(
                    view.m, proj.m,
                    (ImGuizmo::OPERATION)currentOp_,
                    (ImGuizmo::MODE)currentMode_,
                    objectMatrix_,
                    deltaMatrix,
                    snapEnabled_ ? snapValues_ : nullptr)) {

                // Decompose the updated matrix
                math::Vec3 pos, scale;
                math::Quat rot{1.0f, 0.0f, 0.0f, 0.0f};
                decomposeMatrix(objectMatrix_, pos, rot, scale);

                // Clamp scale to avoid singularities / negative scale
                scale.x = std::max(scale.x, 0.001f);
                scale.y = std::max(scale.y, 0.001f);
                scale.z = std::max(scale.z, 0.001f);

                // Write back to ECS — skew is now baked into the decomposed pos/rot/scale,
                // so remove the Skew component to avoid double-application next frame.
                applyToTransform(registry, selectedEntity, pos, rot, scale);
                if (registry.has<components::Skew>(selectedEntity)) {
                    registry.remove<components::Skew>(selectedEntity);
                }
            }
        }
    } else {
        matrixValid_ = false;
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool ImGuizmoSystem::getCameraMatrices(const entities::Registry& registry,
                                        math::Mat4& view, math::Mat4& proj,
                                        float aspect) const {
    for (auto e : registry.view<components::Camera, components::Transform>()) {
        const auto& cam  = registry.get<components::Camera>(e);
        const auto& xform = registry.get<components::Transform>(e);

        math::Vec3 forward = (xform.rotation * math::Vec3{0.0f, 0.0f, -1.0f}).norm();
        math::Vec3 up      = (xform.rotation * math::Vec3{0.0f, 1.0f,  0.0f}).norm();

        view = math::Mat4::lookAt(xform.position, xform.position + forward, up);
        proj = math::Mat4::perspective(cam.fov_y_radians, aspect, cam.near_plane, cam.far_plane);
        return true;
    }
    return false;
}

void ImGuizmoSystem::buildObjectMatrix(const entities::Registry& registry, entities::Entity e,
                                        float* outMatrix) const {
    const auto& xform = registry.get<components::Transform>(e);

    if (registry.has<components::Skew>(e)) {
        const auto& sk = registry.get<components::Skew>(e);
        math::Mat4 M = math::Mat4::modelTRS(xform.position, xform.rotation, xform.scale, sk.shear);
        std::memcpy(outMatrix, M.m, sizeof(M.m));
    } else {
        math::Mat4 M = math::Mat4::modelTRS(xform.position, xform.rotation, xform.scale);
        std::memcpy(outMatrix, M.m, sizeof(M.m));
    }
}

void ImGuizmoSystem::decomposeMatrix(const float* matrix,
                                      math::Vec3& pos, math::Quat& rot, math::Vec3& scale) const {
    // Column-major: m[col*4 + row]
    // Extract translation (column 3)
    pos.x = matrix[12];
    pos.y = matrix[13];
    pos.z = matrix[14];

    // Extract scale as magnitude of each column's 3x3 part
    scale.x = std::sqrt(matrix[0]*matrix[0] + matrix[1]*matrix[1] + matrix[2]*matrix[2]);
    scale.y = std::sqrt(matrix[4]*matrix[4] + matrix[5]*matrix[5] + matrix[6]*matrix[6]);
    scale.z = std::sqrt(matrix[8]*matrix[8] + matrix[9]*matrix[9] + matrix[10]*matrix[10]);

    // Guard against zero scale
    float invSx = (scale.x > 1e-8f) ? 1.0f / scale.x : 1.0f;
    float invSy = (scale.y > 1e-8f) ? 1.0f / scale.y : 1.0f;
    float invSz = (scale.z > 1e-8f) ? 1.0f / scale.z : 1.0f;

    // Build normalised rotation matrix (remove scale from columns)
    float rotMat[16] = {};
    rotMat[0]  = matrix[0]  * invSx;  rotMat[4]  = matrix[4]  * invSy;  rotMat[8]  = matrix[8]  * invSz;
    rotMat[1]  = matrix[1]  * invSx;  rotMat[5]  = matrix[5]  * invSy;  rotMat[9]  = matrix[9]  * invSz;
    rotMat[2]  = matrix[2]  * invSx;  rotMat[6]  = matrix[6]  * invSy;  rotMat[10] = matrix[10] * invSz;
    // rest unused for quaternion extraction

    rot = math::Quat::fromRotationMatrix(rotMat);
}

void ImGuizmoSystem::applyToTransform(entities::Registry& registry, entities::Entity e,
                                       const math::Vec3& pos, const math::Quat& rot,
                                       const math::Vec3& scale) const {
    auto& xform = registry.get<components::Transform>(e);
    xform.position = pos;
    xform.rotation = rot.norm();
    xform.scale    = scale;
}

// ---------------------------------------------------------------------------
// setSkewMode — enable skew editing, auto-add Skew component if needed
// ---------------------------------------------------------------------------

void ImGuizmoSystem::setSkewMode(entities::Registry& registry) {
    skewMode_ = true;

    // If any entity is selected, ensure it has a Skew component
    for (auto e : registry.view<components::Selected>()) {
        if (!registry.has<components::Skew>(e)) {
            registry.emplace<components::Skew>(e);
        }
        break;
    }
}

// ---------------------------------------------------------------------------
// pickEntity — raycast from camera through mouse to select a mesh
// ---------------------------------------------------------------------------

void ImGuizmoSystem::pickEntity(entities::Registry& registry, const core::Window& window) const {
    // --- Screen-space mouse position ---
    float fmx, fmy;
    SDL_GetMouseState(&fmx, &fmy);
    int mx = (int)fmx;
    int my = (int)fmy;

    int vpW, vpH; float aspect;
    window.getDimensions(vpW, vpH, aspect);

    // --- Camera ray ---
    math::Vec3 camPos, camForward, camRight, camUp;
    float fovY, nearPlane;
    bool haveCam = false;
    for (auto e : registry.view<components::Camera, components::Transform>()) {
        const auto& cam  = registry.get<components::Camera>(e);
        const auto& xform = registry.get<components::Transform>(e);
        camPos     = xform.position;
        camForward = (xform.rotation * math::Vec3{0,0,-1}).norm();
        camRight   = (xform.rotation * math::Vec3{1,0, 0}).norm();
        camUp      = (xform.rotation * math::Vec3{0,1, 0}).norm();
        fovY       = cam.fov_y_radians;
        nearPlane  = cam.near_plane;
        haveCam    = true;
        break;
    }
    if (!haveCam) return;

    // NDC from pixel coords (origin top-left in SDL)
    float ndcX = (2.0f * mx) / (float)vpW - 1.0f;
    float ndcY = 1.0f - (2.0f * my) / (float)vpH;

    // Compute ray through the pixel at the near plane
    float halfH = std::tan(fovY * 0.5f) * nearPlane;
    float halfW = halfH * aspect;

    math::Vec3 rayOrigin = camPos;
    math::Vec3 rayDir = (camForward * nearPlane +
                         camRight   * (ndcX * halfW) +
                         camUp      * (ndcY * halfH)).norm();

    // --- Iterate all renderable entities ---
    entities::Entity bestEntity{};
    float bestT = 1e30f;

    for (auto e : registry.view<components::Transform, components::Renderable>()) {
        if (registry.has<components::Disabled>(e)) continue;

        auto& r = registry.get<components::Renderable>(e);
        const graphics::Mesh* mesh = ctx_.mesh_manager.getMesh(r.mesh);
        if (!mesh || mesh->vertices.empty()) continue;

        // Build model matrix
        math::Mat4 model;
        if (registry.has<components::Skew>(e)) {
            const auto& xform = registry.get<components::Transform>(e);
            const auto& sk    = registry.get<components::Skew>(e);
            model = math::Mat4::modelTRS(xform.position, xform.rotation, xform.scale, sk.shear);
        } else {
            const auto& xform = registry.get<components::Transform>(e);
            model = math::Mat4::modelTRS(xform.position, xform.rotation, xform.scale);
        }

        // Compute object-space AABB from mesh vertices
        math::Vec3 obbMin{ 1e30f, 1e30f, 1e30f};
        math::Vec3 obbMax{-1e30f,-1e30f,-1e30f};
        for (const auto& v : mesh->vertices) {
            obbMin.x = std::min(obbMin.x, v.position.x);
            obbMin.y = std::min(obbMin.y, v.position.y);
            obbMin.z = std::min(obbMin.z, v.position.z);
            obbMax.x = std::max(obbMax.x, v.position.x);
            obbMax.y = std::max(obbMax.y, v.position.y);
            obbMax.z = std::max(obbMax.z, v.position.z);
        }

        // Transform 8 AABB corners to world space
        math::Vec3 corners[8] = {
            {obbMin.x, obbMin.y, obbMin.z},
            {obbMax.x, obbMin.y, obbMin.z},
            {obbMin.x, obbMax.y, obbMin.z},
            {obbMax.x, obbMax.y, obbMin.z},
            {obbMin.x, obbMin.y, obbMax.z},
            {obbMax.x, obbMin.y, obbMax.z},
            {obbMin.x, obbMax.y, obbMax.z},
            {obbMax.x, obbMax.y, obbMax.z},
        };

        math::Vec3 wbbMin{ 1e30f, 1e30f, 1e30f};
        math::Vec3 wbbMax{-1e30f,-1e30f,-1e30f};
        for (int i = 0; i < 8; ++i) {
            // Transform corner by model matrix: p' = M * p
            float cx = corners[i].x, cy = corners[i].y, cz = corners[i].z;
            float wx = model.m[0]*cx + model.m[4]*cy + model.m[8] *cz + model.m[12];
            float wy = model.m[1]*cx + model.m[5]*cy + model.m[9] *cz + model.m[13];
            float wz = model.m[2]*cx + model.m[6]*cy + model.m[10]*cz + model.m[14];

            wbbMin.x = std::min(wbbMin.x, wx);
            wbbMin.y = std::min(wbbMin.y, wy);
            wbbMin.z = std::min(wbbMin.z, wz);
            wbbMax.x = std::max(wbbMax.x, wx);
            wbbMax.y = std::max(wbbMax.y, wy);
            wbbMax.z = std::max(wbbMax.z, wz);
        }

        // Slab-method ray-AABB test
        float tMin = 0.0f, tMax = 1e30f;
        for (int axis = 0; axis < 3; ++axis) {
            float o = (&rayOrigin.x)[axis];
            float d = (&rayDir.x)[axis];
            float lo = (&wbbMin.x)[axis];
            float hi = (&wbbMax.x)[axis];

            if (std::fabs(d) < 1e-8f) {
                if (o < lo || o > hi) { tMin = 1e30f; break; }
                continue;
            }
            float t0 = (lo - o) / d;
            float t1 = (hi - o) / d;
            if (t0 > t1) std::swap(t0, t1);
            tMin = std::max(tMin, t0);
            tMax = std::min(tMax, t1);
            if (tMin > tMax) break;
        }

        if (tMin <= tMax && tMin < bestT) {
            bestT = tMin;
            bestEntity = e;
        }
    }

    // --- Apply selection ---
    if (registry.valid(bestEntity)) {
        // Remove Selected from all other entities
        for (auto sel : registry.view<components::Selected>()) {
            if (sel != bestEntity) registry.remove<components::Selected>(sel);
        }
        if (!registry.has<components::Selected>(bestEntity)) {
            registry.emplace<components::Selected>(bestEntity);
        }
    }
}

} // namespace systems
} // namespace exd
