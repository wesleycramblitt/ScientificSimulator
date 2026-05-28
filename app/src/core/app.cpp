#include "core/app.hpp"
#include "core/window.hpp"
#include <iostream>

App::App() : isRunning_(false),
    textureManager_(), cubeMapSystem_(&meshManager_, &textureManager_), meshAssetSystem_(&meshManager_), 
    meshManager_(),  primitiveMeshSystem_(&meshManager_), renderSystem_(&textureManager_, &meshManager_),
    gridSystem_(&meshManager_)  
{
    if (!imguiSystem_.init(window_)) {
        std::cerr << "Failed to initialize ImGuiSystem\n";
    }
}

App::~App() {
    imguiSystem_.shutdown();
}

void App::Run() {
    const std::string sceneName = "Default";
    Scene scene = sceneManager_.loadScene(sceneName);
    primitiveMeshSystem_.update(scene.registry, window_);
    cubeMapSystem_.update(scene.registry, window_);
    meshAssetSystem_.update(scene.registry, window_);
    gridSystem_.update(scene.registry, window_);

    isRunning_ = true;
    while (!window_.should_close) {        
        window_.getEvents();


        polygonModeSystem_.update(scene.registry, window_, 1);
        gridSystem_.update(scene.registry, window_);
        //Camera Controller System
        cameraControllerSystem_.update(scene.registry, window_,1); 
        //Physics System
        //Rendering System
        renderSystem_.update(scene.registry, window_, 1);

        // Gizmo overlay (after 3D scene)
        gizmoSystem_.update(scene.registry, window_);

        //ImGui overlay (after 3D, before swap)
        imguiSystem_.update(scene.registry, window_);

        window_.swapBuffers();
    }
}



