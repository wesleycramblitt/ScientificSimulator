#include "core/app.hpp"
#include "core/window.hpp"
#include <iostream>
#include <chrono>
#include <thread>

namespace exd {
namespace core {

App::App() : isRunning_(false),
    cubeMapSystem_(graphicsContext_),
    meshAssetSystem_(graphicsContext_),
    primitiveMeshSystem_(graphicsContext_),
    renderSystem_(graphicsContext_),
    gridSystem_(graphicsContext_),
    fluidX3DSystem_(graphicsContext_),
    gizmoSystem_(graphicsContext_)
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
    scene::Scene scene = sceneManager_.loadScene(sceneName);

    //Systems that should only run once (may change design later)
    primitiveMeshSystem_.update(scene.registry, window_);
    cubeMapSystem_.update(scene.registry, window_);
    meshAssetSystem_.update(scene.registry, window_);
    gridSystem_.update(scene.registry, window_);

    isRunning_ = true;

    using clock = std::chrono::steady_clock;
    const auto fps = 240;
    const auto target_frame = std::chrono::microseconds(1000000 / fps); // ~60 FPS cap
    auto last_frame = clock::now();

    while (!window_.should_close) {
        auto frame_start = clock::now();
        float dt = std::chrono::duration<float>(frame_start - last_frame).count();
        last_frame = frame_start;

        window_.getEvents();


        polygonModeSystem_.update(scene.registry, window_, dt);

        gridSystem_.update(scene.registry, window_);

        cameraControllerSystem_.update(scene.registry, window_, dt);

        renderSystem_.update(scene.registry, window_, dt);

        gizmoSystem_.update(scene.registry, window_);

        fluidX3DSystem_.update(scene.registry, window_, dt);

        imguiSystem_.update(scene.registry, window_);

        window_.swapBuffers();

        // FPS throttle: sleep if frame finished early
        auto elapsed = clock::now() - frame_start;
        if (elapsed < target_frame) {
            std::this_thread::sleep_for(target_frame - elapsed);
        }
    }
}


} // namespace core
} // namespace exd
