#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include <vector>

struct SimulationDomain;
struct FluidX3DSolverConfig;
struct FluidPhysics;
struct SimulationInfo;
struct Transform;
struct Mesh;
class MeshManager;
class LBM;

class FluidX3DSystem {
public:
    FluidX3DSystem();
    explicit FluidX3DSystem(MeshManager* meshManager);
    ~FluidX3DSystem();
    void update(Registry& registry, Window& window, float dt);

    // Expose LBM for other systems (particle, volume)
    LBM* getLBM() const { return lbm_; }

private:
    Mesh createDomainBox(const SimulationDomain& domain, const Transform& xform);
    void createSolver(Registry& registry, Entity e,
                      const SimulationDomain& domain,
                      const FluidX3DSolverConfig& cfg,
                      const FluidPhysics& phys,
                      SimulationInfo& info,
                      const Transform& xform);

    MeshManager* meshManager_ = nullptr;
    LBM*         lbm_ = nullptr;
};
