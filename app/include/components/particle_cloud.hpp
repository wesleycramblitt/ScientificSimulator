#pragma once
#include <vector>
#include <cstdint>

namespace exd {
namespace components {

// Holds particle positions populated by the solver system.
// ParticleRenderTechnique reads this and uploads to its own VBO.
struct ParticleCloud {
    std::vector<float> positions;  // interleaved x, y, z
    int particle_count = 0;
};

} // namespace components
} // namespace exd
