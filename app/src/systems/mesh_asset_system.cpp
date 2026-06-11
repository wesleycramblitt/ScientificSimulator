#include "systems/mesh_asset_system.hpp"
#include "core/window.hpp"
#include "components/renderable.hpp"
#include "assets/file_importer.hpp"

namespace exd {
namespace systems {

MeshAssetSystem::MeshAssetSystem(graphics::GraphicsContext& graphicsContext) : graphicsContext_(graphicsContext) {}

void MeshAssetSystem::update(entities::Registry& registry, core::Window& window) {
 
    for (auto e : registry.view<components::MeshAsset>()) {
        auto mesh_asset = registry.get<components::MeshAsset>(e);
        auto mesh = createMesh(mesh_asset);

        if (mesh_asset.path.size() == 0) continue;
        
        int32_t mesh_handle = graphicsContext_.mesh_manager.create(mesh);
        registry.emplace<components::Renderable>(e, mesh_handle);

        
    }
}

graphics::Mesh MeshAssetSystem::createMesh(components::MeshAsset meshAsset)
{
    graphics::Mesh mesh = assets::FileImporter::loadMeshWithAssimp(meshAsset.path);
    return mesh;
}


} // namespace systems
} // namespace exd
