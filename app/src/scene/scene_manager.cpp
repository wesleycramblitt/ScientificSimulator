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
#include "components/mirror.hpp"
#include "components/fluid_domain.hpp"
#include "components/fluid_physics.hpp"
#include "components/fluidx3d_config.hpp"
#include "fluidx3d.h"
#include "components/simulation_status.hpp"
#include "components/volume_field.hpp"
#include "components/particle_cloud.hpp"

SceneManager::SceneManager() {}
SceneManager::~SceneManager() {}

Scene SceneManager::loadScene(const std::string& scene_name) {
    Scene scene{};
    scene.name = scene_name;

    auto& registry = scene.registry;

    //Load entities, components into reg from scene
    Entity e = registry.create("Camera");
    registry.emplace<Camera>(e);
    registry.emplace<Transform>(e, Vec3(0,45,200) );  // outside domain looking in
    registry.emplace<CameraController>(e);
    //
    // Entity e2 = registry.create();
    // registry.emplace<Transform>(e2);
    // registry.emplace<Cube>(e2, 100.0f);
    //
    // Entity e3 = registry.create();
    // registry.emplace<Transform>(e3, Vec3(5,2,0));
    // registry.emplace<Cube>(e3, 200.0f);

    Entity e4 = registry.create("CubeMap");
    registry.emplace<CubeMap>(e4, "11", true);


    Entity e5 = registry.create("F117");
    registry.emplace<MeshAsset>(e5, "assets/models/F117/F1172.stl");
    registry.emplace<Mirror>(e5);
    registry.emplace<Transform>(e5, Vec3(0, 80, 0), Quat(1,0,0.0,0), Vec3(0.3, 0.3, 0.3));

    // Simulation domain wrapping the F117 (streamwise = Y)
    Entity simEntity = registry.create("WindTunnel");
    registry.emplace<SimulationDomain>(simEntity, 192, 64, 64);
    auto& solverCfg = registry.emplace<FluidX3DSolverConfig>(simEntity);
    solverCfg.extensions = FLUIDX3D_EXT_VOLUME_FORCE;
    registry.emplace<FluidPhysics>(simEntity, 0.005f, 0.05f, 1, 0.0f, 0.0f, 0.0f, 0.0f);
    auto& simInfo = registry.emplace<SimulationInfo>(simEntity);
    simInfo.total_steps = 5000;
    simInfo.steps_per_frame = 10;
    registry.emplace<Transform>(simEntity, Vec3(0, 80, 0),Quat(0,0,0,1), Vec3(1,1,1));
    // registry.emplace<VolumeField>(simEntity);  // enable volume ray-march
    registry.emplace<ParticleCloud>(simEntity);  // particle tracers
    
    // Grid entity (follows camera)
    Entity gridEntity = registry.create("Grid");
    registry.emplace<Grid>(gridEntity, 10.0f, Vec3{0.6f, 0.6f, 0.6f}, Vec3{1.0f, 1.0f, 1.0f});
    registry.emplace<Transform>(gridEntity);
    


    // registry.emplace<Grid>(e, 1, Vec3(0.5,0.5,0.5), Vec3(1,1,1)); 

    return scene;
}
