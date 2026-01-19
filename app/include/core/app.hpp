#pragma once
#include "core/window.hpp"
#include "scene/scene_manager.hpp"

class App {
    public:
        App();
        ~App();

        void Run();
    private:
        bool isRunning_;
        Window window_;
        SceneManager sceneManager_;
};

