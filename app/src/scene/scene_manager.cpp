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

SceneManager::SceneManager() {}
SceneManager::~SceneManager() {}

Scene SceneManager::loadScene(const std::string& scene_name) {
    Scene scene{};
    scene.name = scene_name;

    auto& registry = scene.registry;

    //Load entities, components into reg from scene
    Entity e = registry.create("Camera");
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

    Entity e4 = registry.create("CubeMap");
    registry.emplace<CubeMap>(e4, "7", true);


    Entity e5 = registry.create("Fighter");
    registry.emplace<MeshAsset>(e5, "assets/models/F117/F117.stl");
    registry.emplace<Mirror>(e5);
    registry.emplace<Transform>(e5, Vec3(100,0,-200), Quat(0.707,-0.707,0.0,0.0), Vec3(0.3, 0.3, 0.3));
    
    // Grid entity (follows camera)
    Entity gridEntity = registry.create("Grid");
    registry.emplace<Grid>(gridEntity, 10.0f, Vec3{0.6f, 0.6f, 0.6f}, Vec3{1.0f, 1.0f, 1.0f});
    registry.emplace<Transform>(gridEntity);
    


    // registry.emplace<Grid>(e, 1, Vec3(0.5,0.5,0.5), Vec3(1,1,1)); 

    return scene;
}
