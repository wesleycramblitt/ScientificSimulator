#pragma once
#include <string>
#include "scene/scene.hpp"

class SceneManager {
    public:
        SceneManager();
        ~SceneManager();

        Scene LoadScene(std::string name);

};
