#pragma once
#include "core/window.hpp"
#include "scene/scene_manager.hpp"
#include "systems/render_system.hpp"
#include "systems/camera_controller_system.hpp"

class App {
    public:
        App();
        ~App();

        void Run();
    private:
        bool isRunning_;
        Window window_;
        SceneManager sceneManager_;
        RenderSystem renderSystem_;
        CameraControllerSystem cameraControllerSystem_;
};

