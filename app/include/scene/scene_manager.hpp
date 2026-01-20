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

        Scene LoadScene(const std::string& name);

};
