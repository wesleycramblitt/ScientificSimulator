#include "core/app.hpp"
#include "core/window.hpp"
#include <iostream>

App::App() : isRunning_(false) {}

App::~App() {     }

void App::Run() {
    const std::string sceneName = "Default";
    Scene scene = sceneManager_.loadScene(sceneName);

    isRunning_ = true;
    while (!window_.should_close) {        
        window_.getEvents();
        window_.handleEvents();
        

        //Call Systems
        //Camera Controller System
        //cameraControllerSystem_.Update(scene.registry, window_,1); 
        //Physics System
        //Rendering System
        renderSystem_.update(scene.registry, window_, 1);

        window_.swapBuffers();
    }
}



