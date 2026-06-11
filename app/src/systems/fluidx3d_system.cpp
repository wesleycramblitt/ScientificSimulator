#include "systems/fluidx3d_system.hpp"
#include "graphics/graphics_context.hpp"

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
#include <fstream>
#include <vector>

namespace exd {
namespace systems {

FluidX3DSystem::FluidX3DSystem(graphics::GraphicsContext& graphicsContext) : graphicsContext_(graphicsContext) {}
FluidX3DSystem::~FluidX3DSystem() { delete lbm_; }

// ── solver creation ──────────────────────────────────────────────────────

void FluidX3DSystem::createSolver(entities::Registry& registry, entities::Entity entity,
                                   const components::SimulationDomain& domain,
                                   const components::FluidX3DSolverConfig& /*cfg*/,
                                   const components::FluidPhysics& phys,
                                   components::SimulationInfo& info,
                                    const components::Transform& xform) {
    const uint nx = domain.nx, ny = domain.ny, nz = domain.nz;
    const float nu = phys.nu;

    // Compute force for desired max velocity ~0.05 in +X direction
    const float u_desired = -0.05f;
    const float fx = u_desired * 2e-3f;  // smaller force for stable channel flow
    const uint particles_N = 500000u;  // more particles for better visibility

    lbm_ = new LBM(nx, ny, nz, nu, fx, 0.0f, 0.0f, particles_N);

    // Channel walls on Y/Z for drag → steady parabolic flow profile
    const ulong N = lbm_->get_N();
    for (uint z = 0; z < nz; z++)
        for (uint y = 0; y < ny; y++)
            for (uint x = 0; x < nx; x++) {
                if (y == 0 || y == ny-1 || z == 0 || z == nz-1) {
                    ulong i = (ulong)x + ((ulong)y + (ulong)z * ny) * nx;
                    lbm_->flags[i] = TYPE_S;
                }
            }

    // Find mesh entity (any MeshAsset) and its world-space Transform
    components::Transform* mesh_xform = nullptr;
    std::string stl_path;
    for (auto fe : registry.view<components::MeshAsset, components::Transform>()) {
        stl_path = registry.get<components::MeshAsset>(fe).path;
        mesh_xform = &registry.get<components::Transform>(fe);
        break;
    }

    if (mesh_xform && !stl_path.empty()) {
        // Read STL vertex range to compute native size
        float stl_min[3] = {1e30f,1e30f,1e30f}, stl_max[3] = {-1e30f,-1e30f,-1e30f};
        {
            std::ifstream file(stl_path, std::ios::binary);
            if (file) {
                file.seekg(0, std::ios::end);
                size_t sz = file.tellg();
                file.seekg(0, std::ios::beg);
                std::vector<char> buf(sz);
                file.read(buf.data(), sz);
                if (sz >= 84) {
                    uint32_t ntri = *(uint32_t*)(buf.data() + 80);
                    const char* p = buf.data() + 84;
                    for (uint32_t i = 0; i < ntri && i < 100000; i++, p += 50) {
                        const float* f = (const float*)p;
                        for (int v = 0; v < 3; v++)  // 3 vertices
                            for (int c = 0; c < 3; c++) {  // x,y,z
                                float val = f[3 + v*3 + c];
                                if (val < stl_min[c]) stl_min[c] = val;
                                if (val > stl_max[c]) stl_max[c] = val;
                            }
                    }
                }
            }
        }
        float nx_size = stl_max[0] - stl_min[0];
        float ny_size = stl_max[1] - stl_min[1];
        float nz_size = stl_max[2] - stl_min[2];
        float native_size = std::max(std::max(nx_size, ny_size), nz_size);

        // Convert world position → grid position
        const math::Vec3& dp = xform.position;
        const math::Vec3& ds = xform.scale;
        const math::Quat dr = xform.rotation;

        math::Vec3 rel{mesh_xform->position.x - dp.x,
                 mesh_xform->position.y - dp.y,
                 mesh_xform->position.z - dp.z};
        math::Quat inv_dr{dr.w, -dr.x, -dr.y, -dr.z};
        math::Vec3 rotated = inv_dr * rel;
        math::Vec3 grid_center{rotated.x / ds.x, rotated.y / ds.y, rotated.z / ds.z};
        grid_center.x += (float)nx * 0.5f;
        grid_center.y += (float)ny * 0.5f;
        grid_center.z += (float)nz * 0.5f;

        // Combine mesh transform with STL axis correction.
        // STL files typically have +Z=forward; solver uses +X=forward.
        // 180° around Y maps +Z ↔ -Z (flip) while keeping Y unchanged,
        // which aligns STL's default axes with the solver's expected flow direction.
        // Grid-space orientation: inv(domain_rot) * mesh_rot
        math::Quat grid_rot = inv_dr * mesh_xform->rotation;
        math::Vec3 rx = grid_rot.right(), ry = grid_rot.up(), rz = grid_rot * math::Vec3{0,0,1};
        float3x3 mesh_rot(rx.x, rx.y, rx.z,
                          ry.x, ry.y, ry.z,
                          rz.x, rz.y, rz.z);

        // Scale and STL-centering compensation
        float grid_scale = native_size * mesh_xform->scale.x / ds.x;
        float stl_to_grid = grid_scale / native_size;

        float stl_cx = (stl_min[0] + stl_max[0]) * 0.5f;
        float stl_cy = (stl_min[1] + stl_max[1]) * 0.5f;
        float stl_cz = (stl_min[2] + stl_max[2]) * 0.5f;
        math::Vec3 stl_gc{stl_cx, stl_cy, stl_cz};
        math::Vec3 rot_gc = grid_rot * stl_gc;
        float3 final_center(grid_center.x + rot_gc.x * stl_to_grid,
                            grid_center.y + rot_gc.y * stl_to_grid,
                            grid_center.z + rot_gc.z * stl_to_grid);

        lbm_->voxelize_stl(stl_path, final_center, mesh_rot, grid_scale, TYPE_S);
    } else {
        printf("[LBM] No MeshAsset found — no obstacle\n");
    }
    printf("[LBM] Voxelization done.\n");

    // Verify voxelized cell count
    int solid_count = 0;
    for (ulong i = 0; i < N; i++)
        if (lbm_->flags[i] == TYPE_S) solid_count++;
    printf("[LBM] Total TYPE_S cells: %d / %llu\n", solid_count, (unsigned long long)N);

    printf("[LBM] Ready.\n");
    info.status = components::SimulationStatus::Stopped;
    info.current_step = 0;
}

// ── per-frame update ─────────────────────────────────────────────────────

void FluidX3DSystem::update(entities::Registry& registry, core::Window& window, float /*dt*/) {
    for (auto e : registry.view<components::SimulationDomain, components::FluidX3DSolverConfig, components::FluidPhysics, components::SimulationInfo, components::Transform>()) {
        if (registry.has<components::Disabled>(e)) continue;

        auto& domain  = registry.get<components::SimulationDomain>(e);
        auto& info    = registry.get<components::SimulationInfo>(e);
        auto& xform   = registry.get<components::Transform>(e);

        if (!registry.has<components::Renderable>(e)) {
            graphics::Mesh boxMesh = createDomainBox(domain, xform);
            uint32_t handle = graphicsContext_.mesh_manager.create(boxMesh);
            registry.emplace<components::Renderable>(e, handle);
            createSolver(registry, e, domain,
                         registry.get<components::FluidX3DSolverConfig>(e),
                         registry.get<components::FluidPhysics>(e), info, xform);
        }

        if (window.simulation_mode && lbm_) {
            if (info.status == components::SimulationStatus::Stopped) {
                info.status = components::SimulationStatus::Running;
                // Seed particles on first run (after initialize)
                seedParticles(domain.nx, domain.ny, domain.nz);
            }

            lbm_->run(info.steps_per_frame, info.total_steps);
            info.current_step += info.steps_per_frame;
            lbm_->integrate_particles(info.steps_per_frame, info.total_steps);

            // Upload velocity magnitude to volume texture
            if (registry.has<components::VolumeField>(e)) {
                auto& vf = registry.get<components::VolumeField>(e);
                if (!vf.interop_ready)
                    graphics::VolumeTexture::create(vf, domain.nx, domain.ny, domain.nz);
                if (vf.interop_ready) {
                    lbm_->u.read_from_device();
                    ulong N = lbm_->get_N();
                    std::vector<float> mag(N);
                    for (ulong i = 0; i < N; i++) {
                        float ux = lbm_->u.x[i], uy = lbm_->u.y[i], uz = lbm_->u.z[i];
                        mag[i] = std::sqrt(ux*ux + uy*uy + uz*uz);
                    }
                    graphics::VolumeTexture::upload(vf, domain.nx, domain.ny, domain.nz, mag.data());
                }
            }

        } else if (!window.simulation_mode && info.status == components::SimulationStatus::Running) {
            info.status = components::SimulationStatus::Stopped;
        }
    }
}

void FluidX3DSystem::seedParticles(uint nx, uint ny, uint nz) {
    if (!lbm_ || !lbm_->particles) return;
    lbm_->particles->read_from_device();
    ulong Np = lbm_->particles->length();
    for (ulong i = 0; i < Np; i++) {
        lbm_->particles->x[i] = (float)(nx - 2) - 0.5f * (float)lbm_->get_Nx() + 0.5f;
        lbm_->particles->y[i] = (float)(rand() % (ny*100)) / 100.0f - 2;    // random Y
        lbm_->particles->z[i] = (float)(rand() % (nz*100)) / 100.0f - 2;    // random Z
    }

    lbm_->particles->write_to_device();
    printf("nx=%u, first particle x=%f, y=%f, z=%f\n", 
       nx, lbm_->particles->x[0], lbm_->particles->y[0], lbm_->particles->z[0]);

    lbm_->particles->read_from_device();
    printf("After readback: x=%f, y=%f, z=%f\n",
       lbm_->particles->x[0], lbm_->particles->y[0], lbm_->particles->z[0]);
}

} // namespace systems
} // namespace exd
