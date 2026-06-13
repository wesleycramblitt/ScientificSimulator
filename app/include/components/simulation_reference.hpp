#pragma once
#include <cstdint>

namespace exd {
namespace components {

// Points an entity back to its owning Simulation entity.
// Systems use this to cross-reference SimulationDomain, FluidPhysics, etc.
// from rendering/diagnostic entities that live on separate Registry slots.
struct SimulationReference {
    std::uint32_t simulation_entity_id = 0;
};

} // namespace components
} // namespace exd
