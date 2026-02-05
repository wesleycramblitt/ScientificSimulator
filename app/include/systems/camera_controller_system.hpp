#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "components/camera_controller.hpp"

class CameraControllerSystem {

    public:
        CameraControllerSystem();
        ~CameraControllerSystem();
        void Update(Registry& registry,Window& window, float dt);

};
