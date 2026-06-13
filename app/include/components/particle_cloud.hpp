#pragma once
#include "icomponent.hpp"
#include <vector>
#include <cstdint>

namespace exd {
namespace components {

// Particle data populated by the solver and read by ParticleRenderTechnique.
struct ParticleCloud {
    std::vector<float> positions;   // interleaved x, y, z
    std::vector<float> colors;      // interleaved r, g, b  (velocity magnitude → color)
    int particle_count = 0;
    int max_particles  = 500000;
};

} // namespace components
} // namespace exd
