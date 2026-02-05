#include "scene/scene_manager.hpp"
#include "components/transform.hpp"
#include "math/vec3.hpp"
#include "math/quat.hpp"
#include "components/sphere.hpp"

SceneManager::SceneManager() {}
SceneManager::~SceneManager() {}

Scene SceneManager::loadScene(const std::string& scene_name) {
    Scene scene{};
    scene.name = scene_name;

    auto& registry = scene.registry;

    //Load entities, components into reg from scene
    Entity e = registry.create();
    registry.emplace<Transform>(e, 
            Vec3(0,0,0),
           Quat(1,1,1,1) 
    );

    registry.emplace<Sphere>(e, 2);

    // registry.emplace<Grid>(e, 1, Vec3(0.5,0.5,0.5), Vec3(1,1,1)); 

    return scene;
}
