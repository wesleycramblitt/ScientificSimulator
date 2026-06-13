#pragma once
#include "icomponent.hpp"
#include<cstdint>

namespace exd {
namespace components {

struct FluidX3DSolverConfig {
    uint8_t velocity_set = 19; //D3Q19
    uint8_t collision = 0; 
    uint8_t  precision    = 0;       // 0=FP32, 1=FP16S, 2=FP16C
    uint32_t dx = 1, dy = 1, dz = 1; // multi-GPU subdivisions
    uint32_t extensions   = 0;       // bitmask (EQUILIBRIUM | SUBGRID | ...
};

} // namespace components
} // namespace exd
