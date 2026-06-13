#pragma once
#include <unordered_map>
#include <string>
#include <cstdint>
#include "graphics/render_techniques/uniform_value.hpp"

namespace exd {
namespace graphics {
namespace render_techniques {

// Data bundle for one particle cloud draw call.
// position/count: raw particle vertex data (interleaved x,y,z).
// uniforms:       u_model, u_view, u_proj
struct ParticleDrawData {
    const float* positions;
    int          count;
    std::unordered_map<std::string, UniformValue> uniforms;
};

} // namespace render_techniques
} // namespace graphics
} // namespace exd
