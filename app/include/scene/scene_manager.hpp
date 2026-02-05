#pragma once
#include <string>
#include "entities/registry.hpp"

struct Scene {
    std::string name;
    Registry registry;
};

class SceneManager {
    public:
         SceneManager();
        ~SceneManager();

        Scene loadScene(const std::string& name);

};
