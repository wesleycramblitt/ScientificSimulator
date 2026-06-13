#pragma once
#include "entities/registry.hpp"
#include "core/window.hpp"
#include <vector>
#include <cstdint>
#include <future>

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
    void destroySolver();

    // Run the expensive solver creation on a background thread.
    // Captures all parameters by value so the main thread can continue.
    void launchAsyncRebuild(entities::Registry& registry,
                            const components::SimulationDomain& domain,
                            const components::FluidPhysics& phys,
                            components::SimulationInfo& info,
                            const components::Transform& xform);

    // Called each frame: if an async rebuild finished, swap in the new solver.
    void checkAsyncRebuild(components::SimulationInfo& info);

    graphics::GraphicsContext& graphicsContext_;

    LBM*         lbm_ = nullptr;

    // Track previous particle X to detect periodic-wrap (X boundaries are periodic)
    std::vector<float> prev_particle_x_;

    // Cache last-known domain dimensions so the box mesh regenerates on edit
    struct { int nx = 0, ny = 0, nz = 0; } domain_cache_;

    // Cache the state the solver was built from, so we can detect
    // runtime edits that need a solver rebuild or re-voxelization.
    struct {
        int nx = 0, ny = 0, nz = 0;
        float pos_x = 0, pos_y = 0, pos_z = 0;
        float mesh_pos_x = 0, mesh_pos_y = 0, mesh_pos_z = 0;
        float mesh_scale = 1.0f;
        float nu = 0.0f;
        float streamwise_velocity = 0.0f;
        uint8_t streamwise_axis = 0;
        int max_particles = 0;
        bool valid = false;
    } solver_cache_;

    // Async rebuild state
    std::future<void> rebuild_future_;
    bool rebuild_in_progress_ = false;

    // Time-based stepping: accumulate real dt, run steps when enough
    // time has passed.  Makes the simulation real-time regardless of FPS.
    float sim_time_accumulator_ = 0.0f;
    static constexpr float target_steps_per_second_ = 300.0f;

    // LBM step at which the next solver health check is due
    uint32_t next_health_check_ = 0;
};

} // namespace systems
} // namespace exd
