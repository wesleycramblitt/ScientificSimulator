#include "core/app.hpp"
#include "core/window.hpp"
#include <iostream>
#include <chrono>

namespace exd {
namespace core {

App::App() : isRunning_(false),
    cubeMapSystem_(graphicsContext_),
    meshAssetSystem_(graphicsContext_),
    primitiveMeshSystem_(graphicsContext_),
    renderSystem_(graphicsContext_),
    gridSystem_(graphicsContext_),
    fluidX3DSystem_(graphicsContext_),
    imguizmoSystem_(graphicsContext_)
{
    if (!imguiSystem_.init(window_)) {
        std::cerr << "Failed to initialize ImGuiSystem\n";
    }
    // Wire the gizmo system into ImGuiSystem so it renders during the ImGui frame
    imguiSystem_.setGizmoSystem(&imguizmoSystem_);
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
    auto last_frame = clock::now();

    while (!window_.should_close) {
        auto frame_start = clock::now();
        float dt = std::chrono::duration<float>(frame_start - last_frame).count();
        last_frame = frame_start;

        window_.getEvents();


        polygonModeSystem_.update(scene.registry, window_, dt);

        gridSystem_.update(scene.registry, window_);

        cameraControllerSystem_.update(scene.registry, window_, dt);

        // ── Simulation (runs before render so the camera stays responsive
        //     and render always shows the latest solver state) ──
        fluidX3DSystem_.update(scene.registry, window_, dt);

        // ── Render ──
        renderSystem_.update(scene.registry, window_, dt);

        imguiSystem_.update(scene.registry, window_);

        // ImGuizmo keyboard shortcuts (W/E/R for operation, X for mode)
        // Called after ImGui so we can check ImGui::GetIO().WantCaptureKeyboard
        imguizmoSystem_.update(scene.registry, window_);

        window_.swapBuffers();

        // No manual FPS cap — vsync / swap interval controls frame pacing.
        // If simulation takes too long, reduce steps_per_frame in the UI.
    }
}


} // namespace core
} // namespace exd
