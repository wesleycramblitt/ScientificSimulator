#pragma once
#include <string>
#include "entities/registry.hpp"

namespace exd {
namespace scene {

struct Scene {
    std::string name;
    entities::Registry registry;
};

class SceneManager {
    public:
         SceneManager();
        ~SceneManager();

        Scene loadScene(const std::string& name);

};

} // namespace scene
} // namespace exd
