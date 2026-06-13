#pragma once
#include <variant>
#include "math/vec3.hpp"
#include "math/quat.hpp"
#include "math/mat4.hpp"

namespace exd {
namespace graphics {
namespace render_techniques {

using UniformValue = std::variant<
    int,
    float,
    math::Vec3,
    math::Quat,
    math::Mat4
>;

} // namespace render_techniques
} // namespace graphics
} // namespace exd
