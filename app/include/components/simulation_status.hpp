#pragma once
#include "icomponent.hpp"
#include<cstdint>

namespace exd {
namespace components {

enum SimulationStatus { Running, Stopped, Completed, Error };
struct SimulationInfo {
    SimulationStatus status = Stopped;
    uint32_t current_step, total_steps, steps_per_frame;
};

} // namespace components
} // namespace exd
