#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include "graphics/mesh_manager.hpp"
#include "graphics/mesh.hpp"

struct SimulationDomain;
struct FluidX3DSolverConfig;
struct FluidPhysics;
struct SimulationInfo;
struct Transform;
struct FluidX3D_Solver;

class FluidX3DSystem {
public:
    FluidX3DSystem();
    explicit FluidX3DSystem(MeshManager* meshManager);
    void update(Registry& registry, Window& window, float dt);

private:
    Mesh createDomainBox(const SimulationDomain& domain, const Transform& xform);
    void createSolver(Registry& registry, Entity e,
                      const SimulationDomain& domain,
                      const FluidX3DSolverConfig& cfg,
                      const FluidPhysics& phys,
                      SimulationInfo& info,
                      const Transform& xform);
    void readbackVelocity(const SimulationDomain& domain);

    MeshManager*   meshManager_ = nullptr;
    FluidX3D_Solver* solver_ = nullptr;
};
