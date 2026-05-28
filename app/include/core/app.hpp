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
        PolygonModeSystem polygonModeSystem_;
        CubeMapSystem cubeMapSystem_;
        MeshAssetSystem meshAssetSystem_;
        GridSystem gridSystem_;
        ImGuiSystem imguiSystem_;
        GizmoSystem gizmoSystem_;
};

