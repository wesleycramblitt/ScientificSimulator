#pragma once
#include "core/window.hpp"
#include "scene/scene_manager.hpp"
#include "systems/render_system.hpp"
#include "systems/camera_controller_system.hpp"
#include "systems/primitive_mesh_system.hpp"
#include "systems/cubemap_system.hpp"
#include "systems/polygon_mode_system.hpp"
#include "systems/mesh_asset_system.hpp"
#include "systems/grid_system.hpp"
#include "systems/imgui_system.hpp"
#include "systems/gizmo_system.hpp"
#include "systems/fluidx3d_system.hpp"
#include "systems/volume_render_system.hpp"
#include "systems/particle_system.hpp"

namespace exd {
namespace core {

class App {
    public:
        App();
        ~App();

        void Run();
    private:
        bool isRunning_;
        Window window_;

        graphics::GraphicsContext graphicsContext_;
        scene::SceneManager sceneManager_;

        systems::RenderSystem renderSystem_;
        systems::PrimitiveMeshSystem primitiveMeshSystem_;
        systems::CameraControllerSystem cameraControllerSystem_;
        systems::PolygonModeSystem polygonModeSystem_;
        systems::CubeMapSystem cubeMapSystem_;
        systems::MeshAssetSystem meshAssetSystem_;
        systems::GridSystem gridSystem_;
        systems::ImGuiSystem imguiSystem_;
        systems::GizmoSystem gizmoSystem_;
        systems::FluidX3DSystem fluidX3DSystem_;
        systems::VolumeRenderSystem volumeRenderSystem_;
        systems::ParticleSystem particleSystem_;
};

} // namespace core
} // namespace exd

