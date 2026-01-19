#include "scene/scene_manager.hpp"

SceneManager::SceneManager() {}
SceneManager::~SceneManager() {}

Scene SceneManager::LoadScene(std::string scene_name) {
    Registry reg;

    //Load entities, components into reg from scene

    Scene scene = { .name = scene_name, .registry= reg } ; 
    return scene;
}
