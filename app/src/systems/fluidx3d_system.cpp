#include "systems/fluidx3d_system.hpp"
#include "graphics/mesh_manager.hpp"

#define Mesh F3D_Mesh
#include "lbm.hpp"
#include "defines.hpp"
#undef Mesh

#include "components/transform.hpp"
#include "components/simulation_status.hpp"
#include "components/fluidx3d_config.hpp"
#include "components/fluid_domain.hpp"
#include "components/fluid_physics.hpp"
#include "components/renderable.hpp"
#include "components/disabled.hpp"
#include "components/volume_field.hpp"
#include "components/mesh_asset.hpp"
#include "graphics/volume_texture.hpp"
#include "math/quat.hpp"

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <vector>

FluidX3DSystem::FluidX3DSystem() = default;
FluidX3DSystem::FluidX3DSystem(MeshManager* meshManager) : meshManager_(meshManager) {}
FluidX3DSystem::~FluidX3DSystem() { delete lbm_; }

// ── solver creation ──────────────────────────────────────────────────────

void FluidX3DSystem::createSolver(Registry& registry, Entity entity,
                                   const SimulationDomain& domain,
                                   const FluidX3DSolverConfig& /*cfg*/,
                                   const FluidPhysics& phys,
                                   SimulationInfo& info,
                                    const Transform& xform) {
    const uint nx = domain.nx, ny = domain.ny, nz = domain.nz;
    const float nu = phys.nu;

    // Compute force for desired max velocity ~0.05 in -X direction
    const float u_desired = 0.05f;
    const float fx = -u_desired * 1e-3f;
    const uint particles_N = 50000u;  // number of tracer particles
    printf("[LBM] Creating %ux%ux%u nu=%.4f force=(%.2e,0,0) particles=%u\n",
           nx, ny, nz, nu, fx, particles_N);

    lbm_ = new LBM(nx, ny, nz, nu, fx, 0.0f, 0.0f, particles_N);
    printf("[LBM] Solver created.\n");

    // ── channel walls (no-slip on Y/Z, flow in X direction) ──
    const ulong N = lbm_->get_N();
    for (uint z = 0; z < nz; z++)
        for (uint y = 0; y < ny; y++)
            for (uint x = 0; x < nx; x++) {
                if (y == 0 || y == ny-1 || z == 0 || z == nz-1) {
                    ulong i = (ulong)x + ((ulong)y + (ulong)z * ny) * nx;
                    lbm_->flags[i] = TYPE_S;
                }
            }
    printf("[LBM] Walls set.\n");

    // ── voxelize F117 ──
    printf("[LBM] Voxelizing F117...\n");

    // Find F117 entity and its world-space Transform
    Transform* f117_xform = nullptr;
    for (auto fe : registry.view<MeshAsset, Transform>()) {
        f117_xform = &registry.get<Transform>(fe);
        break;
    }

    if (f117_xform) {
        // Convert world position → grid position
        // Grid maps to world via: world = domain_rot * (grid * domain_scale) + domain_pos
        // Inverse: grid = inv_domain_rot * ((world - domain_pos) / domain_scale)
        const Vec3& dp = xform.position;   // domain position
        const Vec3& ds = xform.scale;      // domain scale
        const Quat dr = xform.rotation;  // mutable copy

        Vec3 rel = Vec3{f117_xform->position.x - dp.x,
                         f117_xform->position.y - dp.y,
                         f117_xform->position.z - dp.z};
        // Inverse rotation (for unit quaternion, inverse = conjugate)
        Quat inv_dr{dr.w, -dr.x, -dr.y, -dr.z};
        Vec3 grid_center = inv_dr * rel;
        grid_center.x /= ds.x; grid_center.y /= ds.y; grid_center.z /= ds.z;

        // F117 rotation matrix for voxelize_stl
        Quat fq = f117_xform->rotation;  // mutable copy (right/up not const)
        Vec3 rx = fq.right();
        Vec3 ry = fq.up();
        Vec3 rz = fq * Vec3{0,0,1};  // +Z axis after rotation
        float3x3 f117_rot(rx.x, rx.y, rx.z,
                          ry.x, ry.y, ry.z,
                          rz.x, rz.y, rz.z);

        // Scale: STL longest side ≈ 224.7 native units. Convert to grid cells.
        const float native_size = 224.7f;
        float grid_scale = native_size * f117_xform->scale.x / ds.x;

        printf("[LBM] F117 grid pos=(%.1f,%.1f,%.1f) scale=%.1f\n",
               grid_center.x, grid_center.y, grid_center.z, grid_scale);
        lbm_->voxelize_stl("assets/models/F117/F117.stl",
                            float3(grid_center.x, grid_center.y, grid_center.z),
                            f117_rot, grid_scale, TYPE_S);
    } else {
        // Fallback: center of domain
        lbm_->voxelize_stl("assets/models/F117/F117.stl",
                            float3((float)nx*0.5f, (float)ny*0.5f, (float)nz*0.5f),
                            0.3f, TYPE_S);
    }
    printf("[LBM] Voxelization done.\n");

    // Seed particles at +X inlet face
    printf("[LBM] Seeding particles at +X inlet...\n");
    lbm_->particles->read_from_device();
    ulong Np = lbm_->particles->length();
    for (ulong i = 0; i < Np; i++) {
        lbm_->particles->x[i] = (float)(nx - 0.002);                      // near +X face
        lbm_->particles->y[i] = (float)(rand() % (ny*100)) / 100.0f;  // random Y
        lbm_->particles->z[i] = (float)(rand() % (nz*100)) / 100.0f;  // random Z
    }
    lbm_->particles->write_to_device();
    printf("[LBM] Particles seeded.\n");

    printf("[LBM] Ready.\n");
    info.status = SimulationStatus::Stopped;
    info.current_step = 0;
}

// ── per-frame update ─────────────────────────────────────────────────────

void FluidX3DSystem::update(Registry& registry, Window& window, float /*dt*/) {
    for (auto e : registry.view<SimulationDomain, FluidX3DSolverConfig, FluidPhysics, SimulationInfo, Transform>()) {
        if (registry.has<Disabled>(e)) continue;

        auto& domain  = registry.get<SimulationDomain>(e);
        auto& info    = registry.get<SimulationInfo>(e);
        auto& xform   = registry.get<Transform>(e);

        if (!registry.has<Renderable>(e)) {
            Mesh boxMesh = createDomainBox(domain, xform);
            uint32_t handle = meshManager_->create(boxMesh);
            registry.emplace<Renderable>(e, handle);
            createSolver(registry, e, domain,
                         registry.get<FluidX3DSolverConfig>(e),
                         registry.get<FluidPhysics>(e), info, xform);
        }

        if (window.simulation_mode && lbm_) {
            if (info.status == SimulationStatus::Stopped)
                info.status = SimulationStatus::Running;

            lbm_->run(info.steps_per_frame, info.total_steps);
            info.current_step += info.steps_per_frame;
            lbm_->integrate_particles(info.steps_per_frame, info.total_steps);

            // Upload velocity magnitude to volume texture
            if (registry.has<VolumeField>(e)) {
                auto& vf = registry.get<VolumeField>(e);
                if (!vf.interop_ready)
                    VolumeTexture::create(vf, domain.nx, domain.ny, domain.nz);
                if (vf.interop_ready) {
                    lbm_->u.read_from_device();
                    ulong N = lbm_->get_N();
                    std::vector<float> mag(N);
                    for (ulong i = 0; i < N; i++) {
                        float ux = lbm_->u.x[i], uy = lbm_->u.y[i], uz = lbm_->u.z[i];
                        mag[i] = std::sqrt(ux*ux + uy*uy + uz*uz);
                    }
                    VolumeTexture::upload(vf, domain.nx, domain.ny, domain.nz, mag.data());
                }
            }

        } else if (!window.simulation_mode && info.status == SimulationStatus::Running) {
            info.status = SimulationStatus::Stopped;
        }
    }
}
