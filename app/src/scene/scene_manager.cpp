#include "scene/scene_manager.hpp"
#include "components/transform.hpp"
#include "components/camera.hpp"
#include "math/vec3.hpp"
#include "math/quat.hpp"
#include "components/sphere.hpp"
#include "components/cube.hpp"
#include "components/cubemap.hpp"
#include "components/camera_controller.hpp"
#include "components/mesh_asset.hpp"
#include "components/grid.hpp"
#include "components/render_technique_mirror.hpp"
#include "components/render_technique_cubemap.hpp"
#include "components/render_technique_lambertian.hpp"
#include "components/fluid_domain.hpp"
#include "components/fluid_physics.hpp"
#include "components/fluidx3d_config.hpp"
#include "components/disabled.hpp"
#include "components/readonly.hpp"
#include "fluidx3d.h"
#include "components/simulation_status.hpp"
#include "components/volume_field.hpp"
#include "components/particle_cloud.hpp"
#include "components/simulation_reference.hpp"

namespace exd {
namespace scene {

SceneManager::SceneManager() {}
SceneManager::~SceneManager() {}

Scene SceneManager::loadScene(const std::string& scene_name) {
    Scene scene{};
    scene.name = scene_name;

    auto& registry = scene.registry;

    //Load entities, components into reg from scene
    entities::Entity e = registry.create("Camera");
    registry.emplace<components::Camera>(e);
    registry.emplace<components::Transform>(e, math::Vec3(0,45,200) );  // outside domain looking in
    registry.emplace<components::CameraController>(e);
    registry.emplace<components::ReadOnly>(e);
    //
    // entities::Entity e2 = registry.create();
    // registry.emplace<components::Transform>(e2);
    // registry.emplace<components::Cube>(e2, 100.0f);
    //
    // entities::Entity e3 = registry.create();
    // registry.emplace<components::Transform>(e3, math::Vec3(5,2,0));
    // registry.emplace<components::Cube>(e3, 200.0f);

    entities::Entity e4 = registry.create("CubeMap");
    registry.emplace<components::CubeMap>(e4, "10", true);
    registry.emplace<components::Render_Technique_CubeMap>(e4);
    registry.emplace<components::Disabled>(e4);

    entities::Entity e5 = registry.create("F117");
    registry.emplace<components::MeshAsset>(e5, "assets/models/F117/F117.stl");
    registry.emplace<components::Render_Technique_Mirror>(e5);
    registry.emplace<components::Transform>(e5, math::Vec3(0, 80, 0), math::Quat(1,0,0,0), math::Vec3(1, 1, 1));

    // ── Simulation (config only: solver, physics, status) ──
    entities::Entity simEntity = registry.create("WindTunnel");

    auto& solverCfg = registry.emplace<components::FluidX3DSolverConfig>(simEntity);
    solverCfg.extensions = FLUIDX3D_EXT_VOLUME_FORCE;
    registry.emplace<components::FluidPhysics>(simEntity,
        0.02f,   // nu — viscosity (higher = more stable, tau=3*nu+0.5=0.56)
        0.15f,   // streamwise_velocity — target flow speed in lattice units
        0,       // streamwise_axis — 0=X (flow in -X direction)
        0.0f,    // fx — volume force computed from duct dimensions at solver creation
        0.0f, 0.0f, 0.0f);

    auto& simInfo = registry.emplace<components::SimulationInfo>(simEntity);
    simInfo.total_steps = 1000000;
    simInfo.steps_per_frame = 30;  // max per frame; time-accumulator drives actual count

    // ── Domain box (defines domain dimensions, world position, and wireframe) ──
    entities::Entity domainBoxEntity = registry.create("WindTunnel Box");
    registry.emplace<components::SimulationDomain>(domainBoxEntity, 250, 80, 128);
    registry.emplace<components::Transform>(domainBoxEntity,
        math::Vec3(-20, 80, 0), math::Quat(1,0,0,0), math::Vec3(1,1,1));
    registry.emplace<components::Render_Technique_Lambertian>(domainBoxEntity);
    registry.emplace<components::SimulationReference>(domainBoxEntity, simEntity.id);

    // ── Volume ray-march proxy (position derived from domain box) ──
    entities::Entity volumeEntity = registry.create("WindTunnel Volume");
    registry.emplace<components::VolumeField>(volumeEntity);
    registry.emplace<components::SimulationReference>(volumeEntity, simEntity.id);

    // ── Particle cloud (position derived from domain box) ──
    entities::Entity particleEntity = registry.create("WindTunnel Particles");
    registry.emplace<components::ParticleCloud>(particleEntity);
    registry.emplace<components::SimulationReference>(particleEntity, simEntity.id);

    entities::Entity gridEntity = registry.create("Grid");
    registry.emplace<components::Grid>(gridEntity, 50.0f, math::Quat{0.4f, 0.4f, 0.4f, 0.4f});
    registry.emplace<components::Transform>(gridEntity);
    registry.emplace<components::Render_Technique_Lambertian>(gridEntity);
    registry.emplace<components::ReadOnly>(gridEntity);



    return scene;
}

} // namespace scene
} // namespace exd
