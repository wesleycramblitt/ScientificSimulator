#include "core/app.hpp"
#include "core/window.hpp"
#include <iostream>

App::App() : isRunning_(false),
    textureManager_(), cubeMapSystem_(&meshManager_, &textureManager_), meshAssetSystem_(&meshManager_), 
    meshManager_(),  primitiveMeshSystem_(&meshManager_), renderSystem_(&textureManager_, &meshManager_)  
{}

App::~App() {     }

void App::Run() {
    const std::string sceneName = "Default";
    Scene scene = sceneManager_.loadScene(sceneName);
    primitiveMeshSystem_.update(scene.registry, window_);
    cubeMapSystem_.update(scene.registry, window_);
    meshAssetSystem_.update(scene.registry, window_);

    isRunning_ = true;
    while (!window_.should_close) {        
        window_.getEvents();


        polygonModeSystem_.update(scene.registry, window_, 1);
        //Call Systems
        //Camera Controller System
        cameraControllerSystem_.update(scene.registry, window_,1); 
        //Physics System
        //Rendering System
        renderSystem_.update(scene.registry, window_, 1);

        window_.swapBuffers();
    }
}



