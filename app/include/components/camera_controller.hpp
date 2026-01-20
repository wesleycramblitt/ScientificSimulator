#pragma once
#include <cstdint>

struct CameraController {
    float move_speed = 6.0f;      // units/sec
    float sprint_mult = 4.0f;
    float mouse_sensitivity = 0.0025f; // radians per pixel
    bool  invert_y = false;

    // Optional: smoothing
    float position_lerp = 0.0f;   // 0 = off
    float rotation_lerp = 0.0f;
};
