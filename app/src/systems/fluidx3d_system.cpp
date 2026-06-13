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
#include "components/particle_cloud.hpp"
#include "components/mesh_asset.hpp"
#include "components/simulation_reference.hpp"
#include "graphics/texture_3d.hpp"
#include "math/quat.hpp"

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <vector>
#include <array>

namespace exd {
namespace systems {

FluidX3DSystem::FluidX3DSystem(graphics::GraphicsContext& graphicsContext) : graphicsContext_(graphicsContext) {}
FluidX3DSystem::~FluidX3DSystem() { delete lbm_; }


void FluidX3DSystem::createSolver(entities::Registry& registry,
                                   const components::SimulationDomain& domain,
                                   const components::FluidPhysics& phys,
                                   components::SimulationInfo& info,
                                   const components::Transform& xform) {
    const uint nx = domain.nx, ny = domain.ny, nz = domain.nz;
    const float nu = phys.nu;

    // Compute force for desired max velocity ~0.05 in +X direction
    const float u_desired = -0.05f;
    const float fx = u_desired * 2e-3f;  // smaller force for stable channel flow
    const uint particles_N = 100000u;

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
    // Lambda: find the first entity that has C+SimulationReference pointing to simId
    auto findSimChild = [&](entities::Entity::id_type simId, auto componentTag) -> entities::Entity {
        using Tag = decltype(componentTag);
        for (auto e : registry.view<Tag, components::SimulationReference>()) {
            if (registry.get<components::SimulationReference>(e).simulation_entity_id == simId)
                return e;
        }
        return entities::Entity{};
    };

    for (auto simEntity : registry.view<components::FluidX3DSolverConfig,
                                          components::FluidPhysics, components::SimulationInfo>()) {
        if (registry.has<components::Disabled>(simEntity)) continue;
        auto& info    = registry.get<components::SimulationInfo>(simEntity);

        // ── Locate the domain box entity (has SimulationDomain, Transform, SimulationReference) ──
        entities::Entity domainBoxEntity = findSimChild(simEntity.id, components::Transform{});
        if (!registry.valid(domainBoxEntity)) continue;

        auto& domain = registry.get<components::SimulationDomain>(domainBoxEntity);
        auto& xform  = registry.get<components::Transform>(domainBoxEntity);

        // Regenerate domain box mesh when dimensions change
        bool first_time = !registry.has<components::Renderable>(domainBoxEntity);
        bool dims_changed = (domain_cache_.nx != domain.nx ||
                             domain_cache_.ny != domain.ny ||
                             domain_cache_.nz != domain.nz);
        if (first_time || dims_changed) {
            graphics::Mesh boxMesh = createDomainBox(domain, xform);
            uint32_t handle = graphicsContext_.mesh_manager.create(boxMesh);
            if (!first_time)
                registry.get<components::Renderable>(domainBoxEntity).mesh = handle;
            else
                registry.emplace<components::Renderable>(domainBoxEntity, handle);
            domain_cache_ = {domain.nx, domain.ny, domain.nz};
        }

        if (first_time)
            createSolver(registry, domain,
                         registry.get<components::FluidPhysics>(simEntity), info, xform);

        if (window.simulation_mode && lbm_) {
            // ── On transition to Running: reset simulation and respawn particles ──
            if (info.status == components::SimulationStatus::Stopped) {
                info.status = components::SimulationStatus::Running;
                info.current_step = 0;
                lbm_->reset();
                lbm_->u.reset(0.0f);   // zero velocity field so flow restarts from rest
                lbm_->rho.reset(1.0f); // reset density to uniform

                uint nx = domain.nx, ny = domain.ny, nz = domain.nz;
                if (lbm_->particles) {
                    lbm_->particles->read_from_device();
                    ulong Np = lbm_->particles->length();
                    for (ulong i = 0; i < Np; ++i) {
                        // Near +X inlet for -X flow, seeded in centered coordinates
                        float r01_y = (float)rand() / (float)RAND_MAX;
                        float r01_z = (float)rand() / (float)RAND_MAX;
                        lbm_->particles->x[i] = 0.5f * (float)nx - 2.0f;
                        lbm_->particles->y[i] = (r01_y * ((float)ny - 4.0f) + 2.0f) - 0.5f * (float)ny;
                        lbm_->particles->z[i] = (r01_z * ((float)nz - 4.0f) + 2.0f) - 0.5f * (float)nz;
                    }
                    lbm_->particles->write_to_device();
                }
            }

            lbm_->run(info.steps_per_frame, info.total_steps);
            info.current_step += info.steps_per_frame;
            lbm_->integrate_particles(info.steps_per_frame, 1.0f);

            // ── Read back particle positions and colour by velocity ──
            entities::Entity particleEntity = findSimChild(simEntity.id, components::ParticleCloud{});
            if (lbm_->particles && registry.valid(particleEntity)) {
                lbm_->particles->read_from_device();
                lbm_->u.read_from_device();

                uint nx = domain.nx, ny = domain.ny, nz = domain.nz;
                ulong Np = lbm_->particles->length();

                auto& pc = registry.get<components::ParticleCloud>(particleEntity);

                const float lx = 0.5f * (float)nx;
                const float ly = 0.5f * (float)ny;
                const float lz = 0.5f * (float)nz;
                const float max_vel = 0.03f;

                auto velColor = [](float mag, float max) -> std::array<float,3> {
                    float t = std::min(mag / max, 1.0f);
                    if      (t < 0.25f) return {0.0f, t*4.0f, 1.0f};
                    else if (t < 0.50f) return {0.0f, 1.0f, 1.0f-(t-0.25f)*4.0f};
                    else if (t < 0.75f) return {(t-0.50f)*4.0f, 1.0f, 0.0f};
                    else                return {1.0f, 1.0f-(t-0.75f)*4.0f, 0.0f};
                };

                pc.particle_count = (int)Np;
                pc.positions.resize(Np * 3);
                pc.colors.resize(Np * 3);
                for (ulong i = 0; i < Np; ++i) {
                    float px = lbm_->particles->x[i];
                    float py = lbm_->particles->y[i];
                    float pz = lbm_->particles->z[i];

                    pc.positions[i*3+0] = px;
                    pc.positions[i*3+1] = py;
                    pc.positions[i*3+2] = pz;

                    int ix = std::clamp((int)(px + lx + 0.5f), 0, (int)nx-1);
                    int iy = std::clamp((int)(py + ly + 0.5f), 0, (int)ny-1);
                    int iz = std::clamp((int)(pz + lz + 0.5f), 0, (int)nz-1);
                    ulong idx = (ulong)ix + ((ulong)iy + (ulong)iz * ny) * nx;

                    float vx = lbm_->u.x[idx], vy = lbm_->u.y[idx], vz = lbm_->u.z[idx];
                    float mag = std::sqrt(vx*vx + vy*vy + vz*vz);
                    auto col = velColor(mag, max_vel);
                    pc.colors[i*3+0] = col[0];  pc.colors[i*3+1] = col[1];  pc.colors[i*3+2] = col[2];
                }
            }

            // Upload velocity magnitude to volume texture (on the separate volume entity)
            entities::Entity volumeEntity = findSimChild(simEntity.id, components::VolumeField{});
            if (registry.valid(volumeEntity)) {
                auto& vf = registry.get<components::VolumeField>(volumeEntity);
                lbm_->u.read_from_device();
                ulong N = lbm_->get_N();
                std::vector<float> mag(N);
                for (ulong i = 0; i < N; i++) {
                    float ux = lbm_->u.x[i], uy = lbm_->u.y[i], uz = lbm_->u.z[i];
                    mag[i] = std::sqrt(ux*ux + uy*uy + uz*uz);
                }

                if (!vf.interop_ready) {
                    graphics::Texture3D tex(domain.nx, domain.ny, domain.nz, mag.data());
                    vf.texture_handle = graphicsContext_.texture_manager.uploadToGPU(tex);
                    vf.interop_ready = true;
                } else {
                    graphics::Texture3D tex(domain.nx, domain.ny, domain.nz, mag.data());
                    graphicsContext_.texture_manager.update(vf.texture_handle, tex);
                }
            }

        } else if (!window.simulation_mode && info.status == components::SimulationStatus::Running) {
            info.status = components::SimulationStatus::Stopped;
            lbm_->reset();
        }
    }
}

} // namespace systems
} // namespace exd
