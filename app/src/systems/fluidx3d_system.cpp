#include "systems/fluidx3d_system.hpp"
#include "components/transform.hpp"
#include "components/simulation_status.hpp"
#include "components/fluidx3d_config.hpp"
#include "components/fluid_domain.hpp"
#include "components/fluid_physics.hpp"
#include "components/renderable.hpp"
#include "components/mesh_asset.hpp"
#include "components/disabled.hpp"
#include "graphics/mesh_manager.hpp"
#include "graphics/vertex.hpp"
#include "graphics/mesh.hpp"

#include "fluidx3d.h"
#include "defines.hpp"

#include <cstdio>
#include <cmath>
#include <vector>

FluidX3DSystem::FluidX3DSystem() = default;
FluidX3DSystem::FluidX3DSystem(MeshManager* meshManager) : meshManager_(meshManager) {}

void FluidX3DSystem::update(Registry& registry, Window& window, float dt) {
    for (auto e : registry.view<SimulationDomain, FluidX3DSolverConfig, FluidPhysics, SimulationInfo, Transform>()) {
        if (registry.has<Disabled>(e)) continue;

        auto& domain  = registry.get<SimulationDomain>(e);
        auto& cfg     = registry.get<FluidX3DSolverConfig>(e);
        auto& phys    = registry.get<FluidPhysics>(e);
        auto& info    = registry.get<SimulationInfo>(e);
        auto& xform   = registry.get<Transform>(e);

        // ---- Create domain wireframe box (once) ----
        if (!registry.has<Renderable>(e)) {
            Mesh boxMesh = createDomainBox(domain, xform);
            printf("[GridMesh] domain %dx%dx%d  boxMesh vertices=%zu topology=%d\n",
                   domain.nx, domain.ny, domain.nz,
                   boxMesh.vertices.size(), (int)boxMesh.topology);
            uint32_t handle = meshManager_->create(boxMesh);
            printf("[GridMesh] meshManager_.create() returned handle=%u\n", handle);
            registry.emplace<Renderable>(e, handle);

            // Also create solver on first frame
            createSolver(registry, e, domain, cfg, phys, info, xform);
        }

        // ---- Step simulation ----
        if (window.simulation_mode && solver_) {
            if (info.status == SimulationStatus::Stopped)
                info.status = SimulationStatus::Running;

            fluidx3d_run(solver_, info.steps_per_frame, info.total_steps);
            info.current_step += info.steps_per_frame;

            // Read back velocity every N steps
            if (info.current_step % 200 == 0) {
                readbackVelocity(domain);
            }
        } else if (!window.simulation_mode && info.status == SimulationStatus::Running) {
            info.status = SimulationStatus::Stopped;
        }
    }
}

// -----------------------------------------------------------------------
// Create domain 3D volumetric grid mesh (grid-space coordinates)
//
// Mesh vertices are in GRID-SPACE: each simulation cell = 1 unit.
// The domain spans (-nx/2,-ny/2,-nz/2) to (+nx/2,+ny/2,+nz/2).
// World-space transformation (position, rotation, scale) is applied
// by the render system via the entity's Transform component.
// -----------------------------------------------------------------------

Mesh FluidX3DSystem::createDomainBox(const SimulationDomain& domain, const Transform& /*xform*/) {
    Mesh mesh;
    mesh.topology = LINES;

    const int nx = domain.nx;
    const int ny = domain.ny;
    const int nz = domain.nz;

    const float hx = (float)nx * 0.5f;
    const float hy = (float)ny * 0.5f;
    const float hz = (float)nz * 0.5f;

    const Vec3 gridColor{0.8f, 0.8f, 0.8f};   // bright green for face-grid lines
    const Vec3 edgeColor{0.0f, 0.0f, 1.0f};    // bright yellow for outer edges

    auto line = [&](float x1,float y1,float z1, float x2,float y2,float z2, const Vec3& c) {
        mesh.vertices.push_back({Vec3{x1,y1,z1}, c});
        mesh.vertices.push_back({Vec3{x2,y2,z2}, c});
    };

    // ---- Face grids: 10 subdivisions per face edge (dim) ----
    const int div = 10;
    const float dx = 2.0f * hx / (float)div;
    const float dy = 2.0f * hy / (float)div;
    const float dz = 2.0f * hz / (float)div;

    // Top / Bottom faces (Y = ±hy)
    for (int f = 0; f < 2; ++f) {
        const float y = f ? hy : -hy;
        for (int i = 0; i <= div; ++i) {
            float x = -hx + (float)i * dx;
            line(x, y, -hz,  x, y, hz, gridColor);          // Z-direction
            float z = -hz + (float)i * dz;
            line(-hx, y, z,  hx, y, z, gridColor);          // X-direction
        }
    }
    // Front / Back faces (Z = ±hz)
    for (int f = 0; f < 2; ++f) {
        const float z = f ? hz : -hz;
        for (int i = 0; i <= div; ++i) {
            float x = -hx + (float)i * dx;
            line(x, -hy, z,  x, hy, z, gridColor);          // Y-direction
            float y = -hy + (float)i * dy;
            line(-hx, y, z,  hx, y, z, gridColor);          // X-direction
        }
    }
    // Left / Right faces (X = ±hx)
    for (int f = 0; f < 2; ++f) {
        const float x = f ? hx : -hx;
        for (int i = 0; i <= div; ++i) {
            float y = -hy + (float)i * dy;
            line(x, y, -hz,  x, y, hz, gridColor);          // Z-direction
            float z = -hz + (float)i * dz;
            line(x, -hy, z,  x, hy, z, gridColor);          // Y-direction
        }
    }

    // ---- Outer edges: 12 lines of the bounding box (bright) ----
    // Added AFTER face grids so they render on top.

    // Bottom face (Y = -hy)
    line(-hx, -hy, -hz,  hx, -hy, -hz, edgeColor);
    line(-hx, -hy,  hz,  hx, -hy,  hz, edgeColor);
    line(-hx, -hy, -hz, -hx, -hy,  hz, edgeColor);
    line( hx, -hy, -hz,  hx, -hy,  hz, edgeColor);
    // Top face (Y = +hy)
    line(-hx,  hy, -hz,  hx,  hy, -hz, edgeColor);
    line(-hx,  hy,  hz,  hx,  hy,  hz, edgeColor);
    line(-hx,  hy, -hz, -hx,  hy,  hz, edgeColor);
    line( hx,  hy, -hz,  hx,  hy,  hz, edgeColor);
    // Vertical pillars
    line(-hx, -hy, -hz, -hx,  hy, -hz, edgeColor);
    line( hx, -hy, -hz,  hx,  hy, -hz, edgeColor);
    line(-hx, -hy,  hz, -hx,  hy,  hz, edgeColor);
    line( hx, -hy,  hz,  hx,  hy,  hz, edgeColor);

    return mesh;
}

// -----------------------------------------------------------------------
// Create FluidX3D solver
// -----------------------------------------------------------------------

void FluidX3DSystem::createSolver(Registry& registry, Entity entity,
                                   const SimulationDomain& domain,
                                   const FluidX3DSolverConfig& cfg,
                                   const FluidPhysics& phys,
                                   SimulationInfo& info,
                                   const Transform& xform) {
    FluidX3D_Config f3dCfg = fluidx3d_default_config();

    f3dCfg.nx = domain.nx;
    f3dCfg.ny = domain.ny;
    f3dCfg.nz = domain.nz;
    f3dCfg.nu = phys.nu;
    f3dCfg.velocity_set = cfg.velocity_set;
    f3dCfg.collision    = cfg.collision;
    f3dCfg.precision    = cfg.precision;
    f3dCfg.dx = cfg.dx;
    f3dCfg.dy = cfg.dy;
    f3dCfg.dz = cfg.dz;
    f3dCfg.extensions = cfg.extensions;
    f3dCfg.total_steps = info.total_steps;

    printf("[FluidX3D] Creating solver %ux%ux%u...\n", f3dCfg.nx, f3dCfg.ny, f3dCfg.nz);
    solver_ = fluidx3d_create(&f3dCfg);
    if (!solver_) { printf("[FluidX3D] FAILED to create solver\n"); return; }
    printf("[FluidX3D] Solver created.\n");

    // Pre-allocate host buffers BEFORE initialize corrupts the heap
    uint32_t nx, ny, nz;
    fluidx3d_get_dims(solver_, &nx, &ny, &nz);
    uint64_t N = (uint64_t)nx * ny * nz;
    uint8_t* flags = (uint8_t*)calloc(N, sizeof(uint8_t));
    float*   u_buf = (float*)calloc(3u * N, sizeof(float));
    printf("[FluidX3D] Pre-allocated %llu cells\n", (unsigned long long)N);

    // Initialize
    fluidx3d_initialize(solver_);
    printf("[FluidX3D] Initialize returned OK\n"); fflush(stdout);

    // ---- Boundary conditions (wind tunnel) ----
    printf("[FluidX3D] BC start\n"); fflush(stdout);
    {
        for (uint32_t z=0; z<nz; z++) for (uint32_t y=0; y<ny; y++) for (uint32_t x=0; x<nx; x++) {
            uint64_t i = x + ((uint64_t)y + (uint64_t)z)*ny*nx;
            if      (y==0)             flags[i] = TYPE_E;
            else if (y==ny-1)          flags[i] = TYPE_E;
            else if (x==0||x==nx-1||z==0||z==nz-1) flags[i] = TYPE_S;
        }
        printf("[FluidX3D] writing flags...\n"); fflush(stdout);
        fluidx3d_write_field(solver_, FLUIDX3D_FIELD_FLAGS, flags, N);
        printf("[FluidX3D] flags written\n"); fflush(stdout);
    }

    // ---- Initial velocity ----
    {
        for (uint64_t i = 0; i < N; i++) {
            u_buf[i + (uint64_t)phys.streamwise_axis * N] = phys.streamwise_velocity;
        }
        fluidx3d_write_field(solver_, FLUIDX3D_FIELD_U, u_buf, 3u * N * sizeof(float));
        printf("[FluidX3D] Velocity set.\n");
    }

    // Don't free — heap corrupted by initialize
    // free(flags); free(u_buf);

    printf("[FluidX3D] Ready.\n");

    info.status = SimulationStatus::Stopped;
    info.current_step = 0;
}

// -----------------------------------------------------------------------
// Read back velocity field for rendering
// -----------------------------------------------------------------------

void FluidX3DSystem::readbackVelocity(const SimulationDomain& domain) {
    if (!solver_) return;
    uint64_t N;
    float* u = fluidx3d_read_field_float(solver_, FLUIDX3D_FIELD_U, &N);
    if (!u) return;

    uint32_t nx = domain.nx, ny = domain.ny, nz = domain.nz;
    uint32_t cx = nx/2, cy = ny/2, cz = nz/2;
    auto sample = [&](uint32_t x, uint32_t y, uint32_t z) {
        uint64_t i = x + ((uint64_t)y + (uint64_t)z * ny) * nx;
        printf("  [%3u,%3u,%3u] u=(%+.4f, %+.4f, %+.4f)  |u|=%.4f\n",
               x, y, z, u[i], u[i+N/3], u[i+2*N/3],
               sqrtf(u[i]*u[i] + u[i+N/3]*u[i+N/3] + u[i+2*N/3]*u[i+2*N/3]));
    };

    printf("Velocity readback (grid %ux%ux%u):\n", nx, ny, nz);
    sample(cx, cy, cz);
    sample(cx, cy/4, cz);
    sample(cx, cy*3/4, cz);
    sample(cx, cy/2, cz/4);
    printf("\n");

    free(u);
}
