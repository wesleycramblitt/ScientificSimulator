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

    // std::cout << "in camera system update" << std::endl;

    Vec3 worldUp{0.0f, 1.0f, 0.0f};
    float dx = -window.event_state.mouseRelX_;
    float dy = -window.event_state.mouseRelY_;

    cameraController->yaw  += dx * cameraController->mouse_sensitivity;
    cameraController->pitch += dy * cameraController->mouse_sensitivity;

    //~89 deg
    const float maxPitch = 1.55f;
    cameraController->pitch = std::clamp(cameraController->pitch, -maxPitch, maxPitch);

    const float twoPi = 6.28318530718f;
    if (cameraController->yaw > twoPi)  cameraController->yaw -= twoPi;
    if (cameraController->yaw < -twoPi) cameraController->yaw += twoPi;


    Quat qYaw = Quat::fromAxisAngle(worldUp, cameraController->yaw);

    Vec3 right{1.0f, 0.0f, 0.0f};
    Vec3 localRight = (qYaw * right).norm();  
    Quat qPitch = Quat::fromAxisAngle(localRight, cameraController->pitch); 
    cam_xform->rotation = (qPitch * qYaw).norm();
    Vec3 camFwd =  (cam_xform->rotation * Vec3{0.0f, 0.0f, -1.0f}).norm();
    Vec3 front = (camFwd - worldUp * camFwd.dot(worldUp)).norm();
    Vec3 up    = worldUp;          

    float s = cameraController->move_speed * 
    (window.event_state.keyboardState_[SDL_SCANCODE_LSHIFT] ? cameraController->sprint_mult : 1.0f);

    Vec3 move{0,0,0};

    const float step = s * dt;
    
    if (window.event_state.keyboardState_[SDL_SCANCODE_W]) move = move + front * step;
    if (window.event_state.keyboardState_[SDL_SCANCODE_S]) move = move - front * step;
    if (window.event_state.keyboardState_[SDL_SCANCODE_A]) move  = move - localRight * step;
    if (window.event_state.keyboardState_[SDL_SCANCODE_D]) move = move + localRight * step;
    if (window.event_state.keyboardState_[SDL_SCANCODE_Q]) move = move - up    * step;
    if (window.event_state.keyboardState_[SDL_SCANCODE_E]) move = move + up    * step;

    cam_xform->position = cam_xform->position + move;

    window.event_state.mouseRelX_ = 0;
    window.event_state.mouseRelY_ = 0;
}
