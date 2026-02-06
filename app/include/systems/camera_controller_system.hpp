#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "components/camera_controller.hpp"
#include "components/camera.hpp"

class CameraControllerSystem {

    public:
        void update(Registry& registry,Window& window, float dt);

};
