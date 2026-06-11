#pragma once

#include <variant>
#include <unordered_map>
#include <cstdint>

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

struct Renderable {
    uint32_t mesh_handle;
    uint32_t texture_handle;
    std::unordered_map<std::string, UniformValue> uniforms;
};

} // namespace render_techniques
} // namespace graphics
} // namespace exd
