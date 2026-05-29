#include "core/app.hpp"
#include "core/window.hpp"
#include <iostream>
#include <chrono>
#include <thread>

App::App() : isRunning_(false),
    textureManager_(), cubeMapSystem_(&meshManager_, &textureManager_), meshAssetSystem_(&meshManager_), 
    meshManager_(),  primitiveMeshSystem_(&meshManager_), renderSystem_(&textureManager_, &meshManager_),
    gridSystem_(&meshManager_), fluidX3DSystem_(&meshManager_),
    volumeRenderSystem_(&meshManager_)  
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
    using clock = std::chrono::steady_clock;
    const auto target_frame = std::chrono::microseconds(16667); // ~60 FPS cap
    auto last_frame = clock::now();
    while (!window_.should_close) {
        auto frame_start = clock::now();
        float dt = std::chrono::duration<float>(frame_start - last_frame).count();
        last_frame = frame_start;

        window_.getEvents();


        polygonModeSystem_.update(scene.registry, window_, 1);
        gridSystem_.update(scene.registry, window_);
        //Camera Controller System
        cameraControllerSystem_.update(scene.registry, window_, dt);
        //Physics System
        //Rendering System
        renderSystem_.update(scene.registry, window_, 1);
        volumeRenderSystem_.update(scene.registry, window_, 1.0f);
        particleSystem_.update(scene.registry, window_, 1.0f, fluidX3DSystem_.getLBM());

        // Gizmo overlay (after 3D scene)
        gizmoSystem_.update(scene.registry, window_);

        // FluidX3D simulation
        fluidX3DSystem_.update(scene.registry, window_, 1.0f);

        //ImGui overlay (after 3D, before swap)
        imguiSystem_.update(scene.registry, window_);

        window_.swapBuffers();

        // FPS throttle: sleep if frame finished early
        auto elapsed = clock::now() - frame_start;
        if (elapsed < target_frame) {
            std::this_thread::sleep_for(target_frame - elapsed);
        }
    }
}



