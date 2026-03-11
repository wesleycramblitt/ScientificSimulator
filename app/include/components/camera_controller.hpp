#pragma once
#include <cstdint>

struct CameraController {
    float move_speed = 0.025f;      // units/sec
    float sprint_mult = 1.0f;
    float mouse_sensitivity = 0.0025f; // radians per pixel
    float yaw;
    float pitch;
};
