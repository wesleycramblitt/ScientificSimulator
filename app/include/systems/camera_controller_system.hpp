#pragma once
#include "entities/registry.hpp"
#include "core/event_state.hpp"
#include "components/camera_controller.hpp"

class CameraControllerSystem {

    public:
        CameraControllerSystem();
        ~CameraControllerSystem();
        void Update(Registry& registry,EventState& eventState, float dt);

    private:


};
