#pragma once
#include "math/vec3.hpp"
#include "math/quat.hpp"

struct Vertex {
    Vec3 position{};
    Vec3 normal {0.0f, 1.0f, 0.0f};
    Vec3 uv {0.0f, 0.0f};
    Quat tangent {1.0f, 0.0f, 0.0f, 1.0f};
    Quat color {0.8f, 0.8f, 0.8f, 1.0f};
};
