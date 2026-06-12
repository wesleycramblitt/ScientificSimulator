#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include <vector>

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
    void createSolver(entities::Registry& registry, entities::Entity e,
                      const components::SimulationDomain& domain,
                      const components::FluidX3DSolverConfig& cfg,
                      const components::FluidPhysics& phys,
                      components::SimulationInfo& info,
                      const components::Transform& xform);
    void seedParticles(uint nx, uint ny, uint nz);

    graphics::GraphicsContext& graphicsContext_;

    LBM*         lbm_ = nullptr;
};

} // namespace systems
} // namespace exd
