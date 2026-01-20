#include "core/app.hpp"
#include "core/window.hpp"
#include <iostream>

App::App() : isRunning_(false) {}

App::~App() {     }

void App::Run() {
    const std::string sceneName = "Default";
    Scene scene = sceneManager_.LoadScene(sceneName);

    isRunning_ = true;
    while (!window_.should_close) {        
        window_.GetEvents();
        window_.HandleEvents();
        
        //Update EventState pointer in registry

        //Call Systems
        //Camera Controller System
        cameraControllerSystem_.Update(scene.registry, window_.event_state,1); 
        //Physics System
        //Rendering System
        renderSystem_.Update(scene.registry, window_.event_state, 1);

        window_.SwapBuffers();
    }
}



