#pragma once
#include "entities/registry.hpp"
#include "components/camera_controller.hpp"

class CameraControllerSystem {

    public:
        CameraControllerSystem();
        ~CameraControllerSystem();
        void Update(Registry& registry, float dt);

    private:


};
