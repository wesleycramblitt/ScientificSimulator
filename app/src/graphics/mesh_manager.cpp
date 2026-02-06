#include "graphics/mesh_manager.hpp"
#include <stdexcept>



const uint32_t MeshManager::create(const Mesh& mesh) {
    //save data from mesh and return a handle representing it
    //go ahead and create vao as well
    uint32_t id = mesh_map_.size()+1;
    mesh_map_.emplace(id, mesh);
    uploadToGPU(mesh);
    return id;
}

const uint32_t MeshManager::uploadToGPU(const Mesh& mesh) {
   uint32_t id = meshgpu_map_.size()+1;
   meshgpu_map_.try_emplace(id, mesh);

   return id; 
}

const uint32_t MeshManager::load(uint32_t asset_handle) {
    //TODO: Later, load mesh from asset and return mesh handle
    //Also load vao
    //uploadToGPU(mesh);
    return 0;
}

const MeshGPU* MeshManager::bind(const uint32_t mesh_handle) {
    if (meshgpu_map_.find(mesh_handle) == meshgpu_map_.end()) {
        throw std::runtime_error("Mesh handle doesn't exist");
    }

    MeshGPU* meshGPU = &meshgpu_map_.at(mesh_handle);
    meshGPU->bind();

    return meshGPU;
}
