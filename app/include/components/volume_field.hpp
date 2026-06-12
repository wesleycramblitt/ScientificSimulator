#pragma once
#include <cstdint>

namespace exd {
namespace components {

// Attached to domain entity to enable volume rendering of a solver field.
// The GPU texture is managed by graphics::TextureManager.
struct VolumeField {
    int field_id   = 1;      // FLUIDX3D_FIELD_U=1 (velocity)
    int component  = 0;      // 0=x, 1=y, 2=z (scalar component to visualize)

    uint32_t texture_handle = 0;   // TextureManager handle
    bool     interop_ready  = false;
};

} // namespace components
} // namespace exd
