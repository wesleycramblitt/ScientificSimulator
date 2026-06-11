#pragma once
#include <cstdint>

namespace exd {
namespace components {

struct CameraController {
    float move_speed = 100.0f;      // units/sec
    float sprint_mult = 1.0f;
    float mouse_sensitivity = 0.0025f; // radians per pixel
    float yaw;
    float pitch;
};

} // namespace components
} // namespace exd
