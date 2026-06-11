#pragma once
#include<cstdint>

namespace exd {
namespace components {

enum SimulationStatus { Running, Stopped, Error };
struct SimulationInfo {
    SimulationStatus status = Stopped;
    uint32_t current_step, total_steps, steps_per_frame;
};

} // namespace components
} // namespace exd
