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
#include <chrono>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <vector>
#include <array>

namespace exd {
namespace systems {

FluidX3DSystem::FluidX3DSystem(graphics::GraphicsContext& graphicsContext) : graphicsContext_(graphicsContext) {}
FluidX3DSystem::~FluidX3DSystem() { delete lbm_; }


void FluidX3DSystem::destroySolver() {
    if (lbm_) {
        delete lbm_;
        lbm_ = nullptr;
    }
    prev_particle_x_.clear();
    solver_cache_.valid = false;
}

// ── Async solver rebuild ──────────────────────────────────────────────────
// Runs the heavy part (LBM construction + voxelization) on a background
// thread so the UI / render loop never freezes during parameter edits.

void FluidX3DSystem::launchAsyncRebuild(entities::Registry& registry,
                                         const components::SimulationDomain& domain,
                                         const components::FluidPhysics& phys,
                                         components::SimulationInfo& info,
                                         const components::Transform& xform) {
    if (rebuild_in_progress_) return;  // already rebuilding

    // Capture everything the background thread needs by value.
    const uint nx = domain.nx, ny = domain.ny, nz = domain.nz;
    const float nu = phys.nu;
    const float target_u = phys.streamwise_velocity;
    const uint8_t axis = phys.streamwise_axis;

    // Find mesh asset path and transform
    std::string stl_path;
    float mesh_wx = 0, mesh_wy = 0, mesh_wz = 0;
    float mesh_sx = 1.0f, mesh_sy = 1.0f, mesh_sz = 1.0f;
    math::Quat mesh_rot{1, 0, 0, 0};
    for (auto fe : registry.view<components::MeshAsset, components::Transform>()) {
        stl_path = registry.get<components::MeshAsset>(fe).path;
        auto& mt = registry.get<components::Transform>(fe);
        mesh_wx = mt.position.x; mesh_wy = mt.position.y; mesh_wz = mt.position.z;
        mesh_sx = mt.scale.x; mesh_sy = mt.scale.y; mesh_sz = mt.scale.z;
        mesh_rot = mt.rotation;
        break;
    }

    // Determine particle count
    uint particles_N = 10000u;
    for (auto e : registry.view<components::ParticleCloud>()) {
        particles_N = static_cast<uint>(registry.get<components::ParticleCloud>(e).max_particles);
        break;
    }

    // Domain box transform
    const float dp_x = xform.position.x, dp_y = xform.position.y, dp_z = xform.position.z;
    const float ds_x = xform.scale.x, ds_y = xform.scale.y, ds_z = xform.scale.z;
    const math::Quat dr = xform.rotation;

    // Compute force
    const float duct_w = static_cast<float>(ny - 2u);
    const float duct_h = static_cast<float>(nz - 2u);
    const float force_mag = units.f_from_u_rectangular_duct(duct_w, duct_h, 1.0f, nu, target_u);
    float fx = 0, fy = 0, fz = 0;
    switch (axis) {
        case 0: fx = -force_mag; break;
        case 1: fy = -force_mag; break;
        case 2: fz = -force_mag; break;
        default: fx = -force_mag; break;
    }

    // Pause any running simulation and destroy old solver NOW on main thread
    if (info.status != components::SimulationStatus::Stopped) {
        info.status = components::SimulationStatus::Stopped;
        if (lbm_) lbm_->reset();
    }
    destroySolver();  // deletes old lbm_, clears prev_particle_x_

    // Update solver cache immediately so we don't trigger another rebuild
    solver_cache_ = {static_cast<int>(nx), static_cast<int>(ny), static_cast<int>(nz),
                     dp_x, dp_y, dp_z,
                     dr.w, dr.x, dr.y, dr.z,
                     ds_x, ds_y, ds_z,
                     mesh_wx, mesh_wy, mesh_wz,
                     mesh_rot.w, mesh_rot.x, mesh_rot.y, mesh_rot.z,
                     mesh_sx, mesh_sy, mesh_sz,
                     nu, target_u, axis, static_cast<int>(particles_N), true};

    rebuild_in_progress_ = true;

    // Launch heavy work on background thread
    rebuild_future_ = std::async(std::launch::async, [this,
            nx, ny, nz, nu, fx, fy, fz, particles_N,
            stl_path, mesh_wx, mesh_wy, mesh_wz, mesh_sx, mesh_rot,
            dp_x, dp_y, dp_z, ds_x, ds_y, ds_z, dr,
            force_mag, target_u]() {

        // ── Build the new LBM ──
        LBM* new_lbm = new LBM(nx, ny, nz, nu, fx, fy, fz, particles_N);

        // Channel walls on Y/Z
        const ulong N = new_lbm->get_N();
        for (uint z = 0; z < nz; z++)
            for (uint y = 0; y < ny; y++)
                for (uint x = 0; x < nx; x++) {
                    if (y == 0 || y == ny - 1 || z == 0 || z == nz - 1) {
                        ulong i = (ulong)x + ((ulong)y + (ulong)z * ny) * nx;
                        new_lbm->flags[i] = TYPE_S;
                    }
                }

        // ── Voxelize STL mesh (if any) ──
        if (!stl_path.empty()) {
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
                            for (int v = 0; v < 3; v++)
                                for (int c = 0; c < 3; c++) {
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

            math::Vec3 rel{mesh_wx - dp_x, mesh_wy - dp_y, mesh_wz - dp_z};
            math::Quat inv_dr{dr.w, -dr.x, -dr.y, -dr.z};
            math::Vec3 rotated = inv_dr * rel;
            math::Vec3 grid_center{rotated.x / ds_x, rotated.y / ds_y, rotated.z / ds_z};
            grid_center.x += (float)nx * 0.5f;
            grid_center.y += (float)ny * 0.5f;
            grid_center.z += (float)nz * 0.5f;

            math::Quat grid_rot = inv_dr * mesh_rot;
            math::Vec3 rx = grid_rot.right(), ry = grid_rot.up(), rz = grid_rot * math::Vec3{0,0,1};
            float3x3 mrot(rx.x, rx.y, rx.z, ry.x, ry.y, ry.z, rz.x, rz.y, rz.z);

            float grid_scale = native_size * mesh_sx / ds_x;
            float stl_to_grid = grid_scale / native_size;
            float stl_cx = (stl_min[0] + stl_max[0]) * 0.5f;
            float stl_cy = (stl_min[1] + stl_max[1]) * 0.5f;
            float stl_cz = (stl_min[2] + stl_max[2]) * 0.5f;
            math::Vec3 stl_gc{stl_cx, stl_cy, stl_cz};
            math::Vec3 rot_gc = grid_rot * stl_gc;
            float3 final_center(grid_center.x + rot_gc.x * stl_to_grid,
                                grid_center.y + rot_gc.y * stl_to_grid,
                                grid_center.z + rot_gc.z * stl_to_grid);

            new_lbm->voxelize_stl(stl_path, final_center, mrot, grid_scale, TYPE_S);
        }

        // ── Store the new solver (main thread won't touch lbm_ while
        //     rebuild_in_progress_ is true) ──
        lbm_ = new_lbm;
        std::printf("[LBM] Async rebuild complete. nu=%.4f target_u=%.4f fx=%.8f\n",
                   nu, target_u, force_mag);
    });
}

void FluidX3DSystem::checkAsyncRebuild(components::SimulationInfo& info) {
    if (!rebuild_in_progress_) return;

    if (rebuild_future_.valid() &&
        rebuild_future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        rebuild_future_.get();  // rethrows any exception from the worker thread

        prev_particle_x_.clear();
        info.status = components::SimulationStatus::Stopped;
        info.current_step = 0;
        next_health_check_ = 0;
        sim_time_accumulator_ = 0.0f;
        rebuild_in_progress_ = false;

        std::printf("[LBM] Async rebuild swapped in.\n");
    }
}

// ── Synchronous solver creation (used on first load) ──────────────────────

void FluidX3DSystem::createSolver(entities::Registry& registry,
                                   const components::SimulationDomain& domain,
                                   const components::FluidPhysics& phys,
                                   components::SimulationInfo& info,
                                   const components::Transform& xform) {
    const uint nx = domain.nx, ny = domain.ny, nz = domain.nz;
    const float nu = phys.nu;

    // Compute driving force for a rectangular duct using the FluidX3D helper.
    // This yields the exact volume force needed to produce the target centerline
    // velocity for the given duct cross-section, viscosity, and density.
    const float target_u = phys.streamwise_velocity;
    const float duct_width  = static_cast<float>(ny - 2u);   // Y cross-section minus walls
    const float duct_height = static_cast<float>(nz - 2u);   // Z cross-section minus walls

    const float force_magnitude =
        units.f_from_u_rectangular_duct(duct_width, duct_height, 1.0f, nu, target_u);

    // Apply force along the configured streamwise axis (negative = forward)
    float fx = 0.0f, fy = 0.0f, fz = 0.0f;
    switch (phys.streamwise_axis) {
        case 0: fx = -force_magnitude; break;  // flow in -X
        case 1: fy = -force_magnitude; break;  // flow in -Y
        case 2: fz = -force_magnitude; break;  // flow in -Z
        default: fx = -force_magnitude; break;
    }

    // Determine particle count from the scene's ParticleCloud component
    uint particles_N = 100000u;
    for (auto e : registry.view<components::ParticleCloud>()) {
        particles_N = static_cast<uint>(registry.get<components::ParticleCloud>(e).max_particles);
        break;
    }

    std::printf(
        "[LBM] nu=%.4f target_u=%.4f duct=%ux%u fx=%.8f particles=%u\n",
        nu, target_u, static_cast<uint>(duct_width), static_cast<uint>(duct_height),
        force_magnitude, particles_N);

    if (target_u > 0.10f)
        std::printf("[LBM] WARNING: target_u=%.4f exceeds 0.10 lu/ts."
                    " LBM Mach limit ~0.15; instability likely above 0.12.\n",
                    target_u);

    lbm_ = new LBM(nx, ny, nz, nu, fx, fy, fz, particles_N);

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

    // Cache solver construction parameters to detect runtime edits
    solver_cache_ = {static_cast<int>(nx), static_cast<int>(ny), static_cast<int>(nz),
                     xform.position.x, xform.position.y, xform.position.z,
                     xform.rotation.w, xform.rotation.x, xform.rotation.y, xform.rotation.z,
                     xform.scale.x, xform.scale.y, xform.scale.z,
                     mesh_xform ? mesh_xform->position.x : 0.0f,
                     mesh_xform ? mesh_xform->position.y : 0.0f,
                     mesh_xform ? mesh_xform->position.z : 0.0f,
                     mesh_xform ? mesh_xform->rotation.w : 1.0f,
                     mesh_xform ? mesh_xform->rotation.x : 0.0f,
                     mesh_xform ? mesh_xform->rotation.y : 0.0f,
                     mesh_xform ? mesh_xform->rotation.z : 0.0f,
                     mesh_xform ? mesh_xform->scale.x : 1.0f,
                     mesh_xform ? mesh_xform->scale.y : 1.0f,
                     mesh_xform ? mesh_xform->scale.z : 1.0f,
                     phys.nu, phys.streamwise_velocity, phys.streamwise_axis,
                     static_cast<int>(particles_N),
                     true};

    info.status = components::SimulationStatus::Stopped;
    info.current_step = 0;
}

// ── per-frame update ─────────────────────────────────────────────────────

void FluidX3DSystem::update(entities::Registry& registry, core::Window& window, float dt) {
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

        // ── Check for completed async rebuilds ──
        checkAsyncRebuild(info);

        // ── Detect runtime edits that need a solver rebuild ──
        if (lbm_ && solver_cache_.valid && !rebuild_in_progress_) {
            auto& phys = registry.get<components::FluidPhysics>(simEntity);

            // Find mesh entity transform for comparison
            float mesh_px = solver_cache_.mesh_pos_x;
            float mesh_py = solver_cache_.mesh_pos_y;
            float mesh_pz = solver_cache_.mesh_pos_z;
            float mesh_rw = solver_cache_.mesh_rot_w, mesh_rx = solver_cache_.mesh_rot_x;
            float mesh_ry = solver_cache_.mesh_rot_y, mesh_rz = solver_cache_.mesh_rot_z;
            float mesh_sx = solver_cache_.mesh_scale_x;
            float mesh_sy = solver_cache_.mesh_scale_y;
            float mesh_sz = solver_cache_.mesh_scale_z;
            for (auto fe : registry.view<components::MeshAsset, components::Transform>()) {
                auto& mt = registry.get<components::Transform>(fe);
                mesh_px = mt.position.x; mesh_py = mt.position.y; mesh_pz = mt.position.z;
                mesh_rw = mt.rotation.w; mesh_rx = mt.rotation.x;
                mesh_ry = mt.rotation.y; mesh_rz = mt.rotation.z;
                mesh_sx = mt.scale.x; mesh_sy = mt.scale.y; mesh_sz = mt.scale.z;
                break;
            }

            bool dims_changed  = (solver_cache_.nx != static_cast<int>(domain.nx) ||
                                  solver_cache_.ny != static_cast<int>(domain.ny) ||
                                  solver_cache_.nz != static_cast<int>(domain.nz));
            bool pos_changed   = (std::abs(solver_cache_.pos_x - xform.position.x) > 1e-4f ||
                                  std::abs(solver_cache_.pos_y - xform.position.y) > 1e-4f ||
                                  std::abs(solver_cache_.pos_z - xform.position.z) > 1e-4f);
            bool rot_changed   = (std::abs(solver_cache_.rot_w - xform.rotation.w) > 1e-4f ||
                                  std::abs(solver_cache_.rot_x - xform.rotation.x) > 1e-4f ||
                                  std::abs(solver_cache_.rot_y - xform.rotation.y) > 1e-4f ||
                                  std::abs(solver_cache_.rot_z - xform.rotation.z) > 1e-4f);
            bool scl_changed   = (std::abs(solver_cache_.scale_x - xform.scale.x) > 1e-4f ||
                                  std::abs(solver_cache_.scale_y - xform.scale.y) > 1e-4f ||
                                  std::abs(solver_cache_.scale_z - xform.scale.z) > 1e-4f);
            bool mesh_rot_changed = (std::abs(solver_cache_.mesh_rot_w - mesh_rw) > 1e-4f ||
                                     std::abs(solver_cache_.mesh_rot_x - mesh_rx) > 1e-4f ||
                                     std::abs(solver_cache_.mesh_rot_y - mesh_ry) > 1e-4f ||
                                     std::abs(solver_cache_.mesh_rot_z - mesh_rz) > 1e-4f);
            bool mesh_scl_changed = (std::abs(solver_cache_.mesh_scale_x - mesh_sx) > 1e-4f ||
                                     std::abs(solver_cache_.mesh_scale_y - mesh_sy) > 1e-4f ||
                                     std::abs(solver_cache_.mesh_scale_z - mesh_sz) > 1e-4f);

            bool mesh_changed  = (std::abs(solver_cache_.mesh_pos_x - mesh_px) > 1e-4f ||
                                  std::abs(solver_cache_.mesh_pos_y - mesh_py) > 1e-4f ||
                                  std::abs(solver_cache_.mesh_pos_z - mesh_pz) > 1e-4f ||
                                  mesh_rot_changed || mesh_scl_changed);

            bool phys_changed  = (std::abs(solver_cache_.nu - phys.nu) > 1e-6f ||
                                  std::abs(solver_cache_.streamwise_velocity -
                                           phys.streamwise_velocity) > 1e-6f ||
                                  solver_cache_.streamwise_axis != phys.streamwise_axis);

            // Find current particle max from ParticleCloud
            int current_max_particles = solver_cache_.max_particles;
            for (auto e : registry.view<components::ParticleCloud>()) {
                current_max_particles = registry.get<components::ParticleCloud>(e).max_particles;
                break;
            }
            bool particles_changed = (solver_cache_.max_particles != current_max_particles);

            if (dims_changed || pos_changed || rot_changed || scl_changed ||
                mesh_changed || phys_changed || particles_changed) {
                launchAsyncRebuild(registry, domain, phys, info, xform);
                std::printf("[LBM] Async rebuild (dims=%d pos=%d rot=%d scl=%d mesh=%d phys=%d parts=%d)\n",
                           dims_changed, pos_changed, rot_changed, scl_changed,
                           mesh_changed, phys_changed, particles_changed);
            }
        }

        // ── Skip simulation while a rebuild is in progress ──
        if (rebuild_in_progress_) continue;

        if (window.simulation_mode && lbm_) {
            // ── Guard: don't run past total_steps ──
            if (info.status == components::SimulationStatus::Completed)
                return;
            if (info.status == components::SimulationStatus::Error)
                return;

            if (info.current_step >= info.total_steps) {
                info.status = components::SimulationStatus::Completed;
                std::printf("[LBM] Completed — reached total_steps=%u\n", info.total_steps);
                return;
            }

            // ── On transition to Running: reset simulation and seed particles ──
            if (info.status == components::SimulationStatus::Stopped) {
                info.status = components::SimulationStatus::Running;
                info.current_step = 0;
                next_health_check_ = 0;
                sim_time_accumulator_ = 0.0f;
                lbm_->reset();
                lbm_->rho.reset(1.0f); // reset density to uniform

                const auto& phys = registry.get<components::FluidPhysics>(simEntity);
                const uint nx = domain.nx, ny = domain.ny, nz = domain.nz;
                const float target_u = phys.streamwise_velocity;

                // Initialize velocity field near free-stream so the flow starts
                // close to steady state instead of accelerating from rest over
                // hundreds of thousands of time steps.
                // NOTE: flags were set on CPU during createSolver() and are
                // authoritative until the first lbm_->run() pushes them to device.
                {
                    const ulong N = lbm_->get_N();
                    const float u_stream = -target_u;  // flow in -X
                    for (ulong i = 0; i < N; ++i) {
                        if (lbm_->flags[i] & TYPE_S) {
                            lbm_->u.x[i] = 0.0f;
                            lbm_->u.y[i] = 0.0f;
                            lbm_->u.z[i] = 0.0f;
                        } else {
                            lbm_->u.x[i] = u_stream;
                            lbm_->u.y[i] = 0.0f;
                            lbm_->u.z[i] = 0.0f;
                        }
                    }
                    lbm_->u.write_to_device();
                }
                if (lbm_->particles) {
                    lbm_->particles->read_from_device();
                    const ulong Np = lbm_->particles->length();
                    const float lx = 0.5f * static_cast<float>(nx);
                    const float ly = 0.5f * static_cast<float>(ny);
                    const float lz = 0.5f * static_cast<float>(nz);
                    const float margin = 2.5f;
                    const float y_min = -ly + margin, y_max = ly - margin;
                    const float z_min = -lz + margin, z_max = lz - margin;

                    // Seed particles only in the upstream region (inlet side)
                    // so they flow toward and past the obstacle.
                    // Re-sample any particle that lands inside a solid cell
                    // into the inlet region instead of the outlet.
                    const float inlet_x = lx - margin;

                    for (ulong i = 0; i < Np; ++i) {
                        const float rx =
                            static_cast<float>(rand()) /
                            static_cast<float>(RAND_MAX);
                        const float ry =
                            static_cast<float>(rand()) /
                            static_cast<float>(RAND_MAX);
                        const float rz =
                            static_cast<float>(rand()) /
                            static_cast<float>(RAND_MAX);

                        // Seed in a narrow band right near the +X inlet.
                        const float injection_depth = 4.0f;
                        float px = inlet_x - rx * injection_depth;
                        float py = y_min + ry * (y_max - y_min);
                        float pz = z_min + rz * (z_max - z_min);

                        // Reject positions in solid cells
                        int ix = static_cast<int>(px + lx + 0.5f);
                        int iy = static_cast<int>(py + ly + 0.5f);
                        int iz = static_cast<int>(pz + lz + 0.5f);
                        if (ix >= 0 && ix < static_cast<int>(nx) &&
                            iy >= 0 && iy < static_cast<int>(ny) &&
                            iz >= 0 && iz < static_cast<int>(nz)) {
                            const ulong idx =
                                static_cast<ulong>(ix) +
                                (static_cast<ulong>(iy) +
                                 static_cast<ulong>(iz) * ny) * nx;
                            if (lbm_->flags[idx] & TYPE_S) {
                                // Re-sample into the injection band near inlet
                                px = inlet_x - rx * injection_depth;
                            }
                        }

                        lbm_->particles->x[i] = px;
                        lbm_->particles->y[i] = py;
                        lbm_->particles->z[i] = pz;
                    }
                    prev_particle_x_.assign(Np, inlet_x);
                    lbm_->particles->write_to_device();
                }
            }

            // ── Time-based stepping: accumulate real dt, run LBM steps ──
            // to stay synchronized with wall-clock time.
            // steps_per_frame acts as a maximum cap, not a fixed count.
            sim_time_accumulator_ += dt;
            const float step_interval = 1.0f / target_steps_per_second_;
            uint32_t steps_to_run = 0u;
            while (sim_time_accumulator_ >= step_interval &&
                   steps_to_run < info.steps_per_frame) {
                steps_to_run++;
                sim_time_accumulator_ -= step_interval;
            }
            // Prevent unrecoverable backlog
            if (sim_time_accumulator_ > step_interval * 3.0f)
                sim_time_accumulator_ = step_interval * 3.0f;

            const uint32_t remaining = info.total_steps - info.current_step;
            const uint32_t steps = std::min(steps_to_run, remaining);

            if (steps > 0u) {
                lbm_->run(steps, info.total_steps);
                info.current_step += steps;

                if (info.current_step >= info.total_steps) {
                    info.status = components::SimulationStatus::Completed;
                    std::printf("[LBM] Completed — reached total_steps=%u\n", info.total_steps);
                }
            }

            // ── Solver health check every ~500 steps ──
            if (info.current_step >= next_health_check_ && info.status != components::SimulationStatus::Error) {
                next_health_check_ = info.current_step + 500u;
                lbm_->u.read_from_device();
                lbm_->rho.read_from_device();
                lbm_->flags.read_from_device();
                const ulong N = lbm_->get_N();

                float u_max = 0.0f, u_sum = 0.0f;
                float rho_min = 1e30f, rho_max = -1e30f;
                size_t nonfinite_count = 0, fluid_count = 0;

                for (ulong i = 0; i < N; ++i) {
                    if (lbm_->flags[i] & TYPE_S) continue;
                    fluid_count++;

                    const float ux = lbm_->u.x[i];
                    const float uy = lbm_->u.y[i];
                    const float uz = lbm_->u.z[i];

                    if (!std::isfinite(ux) || !std::isfinite(uy) || !std::isfinite(uz)) {
                        nonfinite_count++;
                        continue;
                    }

                    const float speed = std::sqrt(ux * ux + uy * uy + uz * uz);
                    u_sum += std::abs(ux);  // streamwise component for mean
                    if (speed > u_max) u_max = speed;

                    const float r = lbm_->rho[i];
                    if (r < rho_min) rho_min = r;
                    if (r > rho_max) rho_max = r;
                }

                const float u_mean = (fluid_count > 0) ? u_sum / static_cast<float>(fluid_count) : 0.0f;

                // Stability thresholds relative to target velocity
                const auto& phys = registry.get<components::FluidPhysics>(simEntity);
                const float u_limit = std::max(phys.streamwise_velocity * 4.0f, 0.15f);

                std::printf(
                    "[LBM] t=%llu health:"
                    " umax=%.4f umean=%.4f rho=[%.4f,%.4f]"
                    " nonfinite=%zu fluid=%zu\n",
                    static_cast<unsigned long long>(lbm_->get_t()),
                    u_max, u_mean, rho_min, rho_max,
                    nonfinite_count, fluid_count);

                if (nonfinite_count > 0 ||
                    u_max > u_limit ||
                    rho_min < 0.8f ||
                    rho_max > 1.2f) {
                    info.status = components::SimulationStatus::Error;
                    std::fprintf(
                        stderr,
                        "[LBM] UNSTABLE at t=%llu step=%u:"
                        " umax=%.4f rho=[%.4f,%.4f]"
                        " nonfinite=%zu\n",
                        static_cast<unsigned long long>(lbm_->get_t()),
                        info.current_step,
                        u_max, rho_min, rho_max,
                        nonfinite_count);
                    return;
                }
            }

            // ── Read back particles, detect wrap, respawn, colour by velocity ──
            entities::Entity particleEntity = findSimChild(simEntity.id, components::ParticleCloud{});
            if (lbm_->particles && registry.valid(particleEntity)) {
                lbm_->particles->read_from_device();
                lbm_->u.read_from_device();

                const uint nx = domain.nx, ny = domain.ny, nz = domain.nz;
                const ulong Np = lbm_->particles->length();

                auto& pc = registry.get<components::ParticleCloud>(particleEntity);

                const float lx = 0.5f * static_cast<float>(nx);
                const float ly = 0.5f * static_cast<float>(ny);
                const float lz = 0.5f * static_cast<float>(nz);
                const float margin = 2.5f;
                const float inlet_x = lx - margin;
                const float injection_depth = 4.0f;  // stochastic X spread for respawns
                const float y_min = -ly + margin, y_max = ly - margin;
                const float z_min = -lz + margin, z_max = lz - margin;
                const float ref_speed = registry.get<components::FluidPhysics>(simEntity).streamwise_velocity;
                bool any_respawned = false;

                if (prev_particle_x_.size() != Np)
                    prev_particle_x_.assign(Np, inlet_x);

                // Diverging colormap: blue (slow) → dim (normal) → red (fast)
                auto velColor = [](float speed, float ref) -> std::array<float,3> {
                    float dev = std::clamp((speed - ref) / ref, -1.0f, 1.0f);
                    if (dev < 0.0f) {
                        // Slower → blue
                        float s = -dev;
                        return {0.0f, 0.15f * s, 0.3f + 0.7f * s};
                    } else {
                        // Faster → red → yellow
                        if (dev < 0.5f)
                            return {1.6f * dev, 0.0f, 0.0f};
                        else
                            return {0.8f + 0.4f * (dev - 0.5f), 0.8f * (dev - 0.5f), 0.0f};
                    }
                };

                pc.particle_count = static_cast<int>(Np);
                pc.positions.resize(Np * 3);
                pc.colors.resize(Np * 3);
                for (ulong i = 0; i < Np; ++i) {
                    float px = lbm_->particles->x[i];
                    float py = lbm_->particles->y[i];
                    float pz = lbm_->particles->z[i];

                    // X boundaries are periodic (flow is -X).  Detect the
                    // discontinuity when a particle wraps from -X to +X.
                    bool nonfinite = !std::isfinite(px) || !std::isfinite(py) || !std::isfinite(pz);
                    bool wrapped   = (px - prev_particle_x_[i] > 0.5f * static_cast<float>(nx));
                    bool oob_yz    = (py < y_min || py > y_max || pz < z_min || pz > z_max);

                    if (nonfinite || wrapped || oob_yz) {
                        const float r01_x =
                            static_cast<float>(rand()) /
                            static_cast<float>(RAND_MAX);
                        const float r01_y =
                            static_cast<float>(rand()) /
                            static_cast<float>(RAND_MAX);
                        const float r01_z =
                            static_cast<float>(rand()) /
                            static_cast<float>(RAND_MAX);

                        px = inlet_x - r01_x * injection_depth;  // spread along X
                        py = y_min + r01_y * (y_max - y_min);
                        pz = z_min + r01_z * (z_max - z_min);
                        lbm_->particles->x[i] = px;
                        lbm_->particles->y[i] = py;
                        lbm_->particles->z[i] = pz;
                        any_respawned = true;
                    }
                    prev_particle_x_[i] = px;

                    pc.positions[i*3+0] = px;
                    pc.positions[i*3+1] = py;
                    pc.positions[i*3+2] = pz;

                    int ix = std::clamp(static_cast<int>(px + lx + 0.5f), 0, static_cast<int>(nx) - 1);
                    int iy = std::clamp(static_cast<int>(py + ly + 0.5f), 0, static_cast<int>(ny) - 1);
                    int iz = std::clamp(static_cast<int>(pz + lz + 0.5f), 0, static_cast<int>(nz) - 1);
                    ulong idx = static_cast<ulong>(ix) + (static_cast<ulong>(iy) + static_cast<ulong>(iz) * ny) * nx;

                    float vx = lbm_->u.x[idx], vy = lbm_->u.y[idx], vz = lbm_->u.z[idx];
                    float mag = std::sqrt(vx*vx + vy*vy + vz*vz);
                    auto col = velColor(mag, ref_speed);
                    pc.colors[i*3+0] = col[0];  pc.colors[i*3+1] = col[1];  pc.colors[i*3+2] = col[2];
                }

                if (any_respawned)
                    lbm_->particles->write_to_device();
            }

            // ── Upload signed speed-deviation scalar to volume texture ──
            // Negative = slower than free-stream (blue), zero = normal
            // (transparent), positive = faster (red).
            entities::Entity volumeEntity = findSimChild(simEntity.id, components::VolumeField{});
            if (registry.valid(volumeEntity)) {
                auto& vf = registry.get<components::VolumeField>(volumeEntity);
                lbm_->u.read_from_device();
                lbm_->flags.read_from_device();
                const ulong N = lbm_->get_N();
                const auto& phys = registry.get<components::FluidPhysics>(simEntity);
                const float ref_speed = phys.streamwise_velocity;
                std::vector<float> scalar(N);
                for (ulong i = 0; i < N; i++) {
                    if (lbm_->flags[i] & TYPE_S) {
                        scalar[i] = 0.0f;
                        continue;
                    }
                    const float ux = lbm_->u.x[i];
                    const float uy = lbm_->u.y[i];
                    const float uz = lbm_->u.z[i];
                    if (!std::isfinite(ux) || !std::isfinite(uy) || !std::isfinite(uz)) {
                        scalar[i] = 0.0f;
                        continue;
                    }
                    const float speed = std::sqrt(ux * ux + uy * uy + uz * uz);
                    // Signed deviation, normalized so ±30% of free-stream
                    // maps to full color; smaller deviations stay near transparent.
                    scalar[i] = std::clamp((speed - ref_speed) / (ref_speed * 0.3f), -1.0f, 1.0f);
                }

                if (!vf.interop_ready) {
                    graphics::Texture3D tex(domain.nx, domain.ny, domain.nz, scalar.data());
                    vf.texture_handle = graphicsContext_.texture_manager.uploadToGPU(tex);
                    vf.interop_ready = true;
                } else {
                    graphics::Texture3D tex(domain.nx, domain.ny, domain.nz, scalar.data());
                    graphicsContext_.texture_manager.update(vf.texture_handle, tex);
                }
            }

        } else if (!window.simulation_mode &&
                   (info.status == components::SimulationStatus::Running ||
                    info.status == components::SimulationStatus::Completed)) {
            info.status = components::SimulationStatus::Stopped;
            if (lbm_) lbm_->reset();
        }
    }
}

} // namespace systems
} // namespace exd
