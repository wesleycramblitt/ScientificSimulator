#pragma once
#include <unordered_map>
#include <string>
#include <cstdint>
#include "graphics/render_techniques/uniform_value.hpp"

namespace exd {
namespace graphics {
namespace render_techniques {

struct ParticleDrawData {
    const float* positions;
    const float* colors;          // may be nullptr for solid-color fallback
    int          count;
    std::unordered_map<std::string, UniformValue> uniforms;
};

} // namespace render_techniques
} // namespace graphics
} // namespace exd
