#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "components/camera_controller.hpp"
#include "components/camera.hpp"

namespace exd {
namespace systems {

class CameraControllerSystem {
    public:
        void update(entities::Registry& registry, core::Window& window, float dt);

};

} // namespace systems
} // namespace exd
