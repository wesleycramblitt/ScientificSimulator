#pragma once
#include <cstdint>

struct CameraController {
    float move_speed = 3.0f;      // units/sec
    float sprint_mult = 4.0f;
    float mouse_sensitivity = 0.0025f; // radians per pixel

};
