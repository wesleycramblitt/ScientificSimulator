#pragma once
#include "core/window.hpp"
#include "scene/scene_manager.hpp"
#include "systems/render_system.hpp"
#include "systems/camera_controller_system.hpp"
#include "systems/primitive_mesh_system.hpp"
#include "systems/cubemap_system.hpp"

class App {
    public:
        App();
        ~App();

        void Run();
    private:
        bool isRunning_;
        Window window_;
        SceneManager sceneManager_;
        MeshManager meshManager_;
        TextureManager textureManager_;
        RenderSystem renderSystem_;
        PrimitiveMeshSystem primitiveMeshSystem_;
        CameraControllerSystem cameraControllerSystem_;
        CubeMapSystem cubeMapSystem_;
};

