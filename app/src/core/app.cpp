#include "core/app.hpp"
#include "core/window.hpp"
#include <iostream>

App::App() : isRunning_(false) {}

App::~App() {     }

void App::Run() {
    Scene scene = sceneManager_.LoadScene("Default");

    isRunning_ = true;
    while (!window_.should_close) {        
        window_.GetEvents();
        window_.HandleEvents();
        
        //Call Systems
        //Camera Controller System
        //Physics System
        //Rendering System

        window_.SwapBuffers();
    }
}



