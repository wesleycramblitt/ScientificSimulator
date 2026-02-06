#include "systems/camera_controller_system.hpp"
#include "core/window.hpp"
#include "math/vec3.hpp"
#include "math/quat.hpp"
#include "components/transform.hpp"
#include <iostream>


void CameraControllerSystem::update(Registry& registry, Window& window, float dt) {

   if (!window.event_state.keyboardState_) return;
    
   Camera* camera = nullptr;
   CameraController* cameraController = nullptr;
   Transform* cam_xform = nullptr;

    for (auto e : registry.view<Camera, CameraController, Transform>()) {
         camera = &registry.get<Camera>(e);
         cameraController = &registry.get<CameraController>(e);
         cam_xform = &registry.get<Transform>(e);
         break;
    }

    if (!camera || !cameraController || ! cam_xform) return;

    std::cout << "in camera system update" << std::endl;

    Vec3 worldUp{0.0f, 1.0f, 0.0f};
    float dx = window.event_state.mouseRelX_;
    float dy = window.event_state.mouseRelY_;

    std::cout << "dx: " << dx << std::endl;
    std::cout << "dy: " << dy << std:: endl;

    const float yawDelta   = dx * cameraController->mouse_sensitivity;
    const float pitchDelta = dy * cameraController->mouse_sensitivity;

    Quat qYaw = Quat::fromAxisAngle(worldUp, yawDelta);
    cam_xform->rotation = (qYaw * cam_xform->rotation).norm();

    Vec3 right = cam_xform->rotation.right().norm();    // or rotate({1,0,0})
    Quat qPitch = Quat::fromAxisAngle(right, -pitchDelta); // minus sign depends on your dy convention

    const float maxUpDot = 0.999f; // ~87-89 degrees; adjust to taste

    Vec3 fwdBefore = cam_xform->rotation.forward().norm();
    Vec3 fwdAfter  = qPitch * cam_xform->rotation.forward().norm();

    if (std::abs(fwdAfter.dot(worldUp)) < maxUpDot) {
        cam_xform->rotation = (qPitch * cam_xform->rotation).norm();
    }

    Vec3 front = cam_xform->rotation.forward().norm();
    Vec3 up = cam_xform->rotation.up().norm();

    float s = cameraController->move_speed * 
        (window.event_state.keyboardState_[SDL_SCANCODE_LSHIFT] ? cameraController->sprint_mult : 1.0f);

    Vec3 move{0,0,0};

    const float step = s * dt;

    std::cout << "step" << step << std::endl;
    
    if (window.event_state.keyboardState_[SDL_SCANCODE_W]) move = move + front * step;
    if (window.event_state.keyboardState_[SDL_SCANCODE_S]) move = move - front * step;
    if (window.event_state.keyboardState_[SDL_SCANCODE_A]) move  = move - right * step;
    if (window.event_state.keyboardState_[SDL_SCANCODE_D]) move = move + right * step;
    if (window.event_state.keyboardState_[SDL_SCANCODE_Q]) move = move - up    * step;
    if (window.event_state.keyboardState_[SDL_SCANCODE_E]) move = move + up    * step;

    cam_xform->position = cam_xform->position + move;
    std::cout << "cam_xform position: " << cam_xform->position.x << std::endl;

    window.event_state.mouseRelX_ = 0;
    window.event_state.mouseRelY_ = 0;
}
