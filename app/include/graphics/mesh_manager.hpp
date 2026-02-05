#pragma once
#include "graphics/mesh.hpp"
#include <unordered_map>

class MeshManager {
public:
    const uint32_t create(const Mesh& mesh);
    const uint32_t uploadToGPU(const Mesh& mesh);
    const uint32_t load(uint32_t asset_handle);
    const MeshGPU bind(const uint32_t mesh);

private:
    uint32_t nextId{};
    std::unordered_map<uint32_t, Mesh> mesh_map_;
    std::unordered_map<uint32_t, MeshGPU> meshgpu_map_;
};
