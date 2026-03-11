#include "systems/mesh_asset_system.hpp"
#include "core/window.hpp"
#include "components/renderable.hpp"
#include "assets/file_importer.hpp"

MeshAssetSystem::MeshAssetSystem(MeshManager* _meshManager) : meshManager_(_meshManager) {}

void MeshAssetSystem::update(Registry& registry, Window& window) {
 
    for (auto e : registry.view<MeshAsset>()) {
        auto mesh_asset = registry.get<MeshAsset>(e);
        auto mesh = createMesh(mesh_asset);

        if (mesh_asset.path.size() == 0) continue;
        
        int32_t mesh_handle = meshManager_->create(mesh);
        registry.emplace<Renderable>(e, mesh_handle);

        
    }
}

Mesh MeshAssetSystem::createMesh(MeshAsset meshAsset)
{
    Mesh mesh = FileImporter::loadMeshWithAssimp(meshAsset.path);
    return mesh;
}


