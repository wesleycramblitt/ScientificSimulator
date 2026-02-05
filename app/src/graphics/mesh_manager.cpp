#include "graphics/mesh_manager.hpp"
#include <stdexcept>



const uint32_t MeshManager::create(const Mesh& mesh) {
    //save data from mesh and return a handle representing it
    //go ahead and create vao as well
    int id = nextId++;
    mesh_map_.emplace(id, mesh);
    uploadToGPU(mesh);
    return id;
}

const uint32_t MeshManager::uploadToGPU(const Mesh& mesh) {
   MeshGPU mesh_gpu;

   //gen vao
   //gen vbo
   //gen ebo
   //

}

const uint32_t MeshManager::load(uint32_t asset_handle) {
    //TODO: Later, load mesh from asset and return mesh handle
    //Also load vao
    //uploadToGPU(mesh);
}

const MeshGPU MeshManager::bind(const uint32_t mesh_handle) {
    if (meshgpu_map_.find(mesh_handle) == meshgpu_map_.end()) {
        throw std::runtime_error("Mesh handle doesn't exist");
    }
    glBindVertexArray(meshgpu_map_[mesh_handle].vao);
}
