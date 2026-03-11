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

SceneManager::SceneManager() {}
SceneManager::~SceneManager() {}

Scene SceneManager::loadScene(const std::string& scene_name) {
    Scene scene{};
    scene.name = scene_name;

    auto& registry = scene.registry;

    //Load entities, components into reg from scene
    Entity e = registry.create();
    registry.emplace<Camera>(e);
    registry.emplace<Transform>(e, Vec3(0,0,5) );
    registry.emplace<CameraController>(e);
    //
    // Entity e2 = registry.create();
    // registry.emplace<Transform>(e2);
    // registry.emplace<Cube>(e2, 100.0f);
    //
    // Entity e3 = registry.create();
    // registry.emplace<Transform>(e3, Vec3(5,2,0));
    // registry.emplace<Cube>(e3, 200.0f);

    Entity e4 = registry.create();
    registry.emplace<CubeMap>(e4, "Daylight");

    Entity e5 = registry.create();
    registry.emplace<MeshAsset>(e5, "assets/models/fighter/fighter.obj");
    registry.emplace<Transform>(e5, Vec3(100,0,-200), Quat(0.707,-0.707,0.0,0.0), Vec3(0.3, 0.3, 0.3));
    


    // registry.emplace<Grid>(e, 1, Vec3(0.5,0.5,0.5), Vec3(1,1,1)); 

    return scene;
}
