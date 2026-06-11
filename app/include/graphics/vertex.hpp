#pragma once
#include "math/vec3.hpp"
#include "math/quat.hpp"

namespace exd {
namespace graphics {

struct Vertex {
    math::Vec3 position{};
    math::Vec3 normal {0.0f, 1.0f, 0.0f};
    math::Vec3 uv {0.0f, 0.0f};
    math::Quat tangent {1.0f, 0.0f, 0.0f, 1.0f};
    math::Quat color {0.8f, 0.8f, 0.8f, 1.0f};
};

} // namespace graphics
} // namespace exd
