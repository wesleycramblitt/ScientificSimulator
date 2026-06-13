#pragma once
#include <unordered_map>
#include <string>
#include <cstdint>
#include "graphics/render_techniques/uniform_value.hpp"

namespace exd {
namespace graphics {
namespace render_techniques {

// Direct handles / dimensions + uniforms for u_model, u_view, u_proj,
// u_cam_pos, u_box_min, u_box_max.
// u_grid_dims is set automatically from nx/ny/nz via glUniform3i.
struct VolumeDrawData {
    uint32_t texture_handle;
    uint32_t proxy_mesh;
    int nx, ny, nz;
    std::unordered_map<std::string, UniformValue> uniforms;
};

} // namespace render_techniques
} // namespace graphics
} // namespace exd
