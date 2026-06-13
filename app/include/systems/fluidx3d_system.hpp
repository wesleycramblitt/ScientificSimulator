#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include <vector>
#include <cstdint>

// External library type (not in exd namespace)
class LBM;

namespace exd {
namespace components {
struct SimulationDomain;
struct FluidX3DSolverConfig;
struct FluidPhysics;
struct SimulationInfo;
struct Transform;
} // namespace components
  //
namespace graphics {
struct Mesh;
class GraphicsContext;
} // namespace graphics
namespace systems {

class FluidX3DSystem {
public:
    explicit FluidX3DSystem(graphics::GraphicsContext& graphicsContext);
    ~FluidX3DSystem();
    void update(entities::Registry& registry, core::Window& window, float dt);

private:
    graphics::Mesh createDomainBox(const components::SimulationDomain& domain, const components::Transform& xform);
    void createSolver(entities::Registry& registry,
                      const components::SimulationDomain& domain,
                      const components::FluidPhysics& phys,
                      components::SimulationInfo& info,
                      const components::Transform& xform);

    graphics::GraphicsContext& graphicsContext_;

    LBM*         lbm_ = nullptr;

    // Track previous particle X to detect periodic-wrap (X boundaries are periodic)
    std::vector<float> prev_particle_x_;

    // Cache last-known domain dimensions so the box mesh regenerates on edit
    struct { int nx = 0, ny = 0, nz = 0; } domain_cache_;
};

} // namespace systems
} // namespace exd
