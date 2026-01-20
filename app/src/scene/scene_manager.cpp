#include "scene/scene_manager.hpp"

SceneManager::SceneManager() {}
SceneManager::~SceneManager() {}

Scene SceneManager::LoadScene(const std::string& scene_name) {
    Scene scene{};
    scene.name = scene_name;

    auto& registry = scene.registry;

    //Load entities, components into reg from scene

    return scene;
}
