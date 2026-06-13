#pragma once
#include "icomponent.hpp"
#include <cstdint>

namespace exd {
namespace components {

// Separate mesh handle for the volume ray-march proxy cube,
// so it doesn't conflict with the grid wireframe Renderable on the same entity.
struct VolumeRenderable {
    uint32_t mesh = 0;   // handle into MeshManager
};

} // namespace components
} // namespace exd
