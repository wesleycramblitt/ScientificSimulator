#include "common/macros.hpp"
#include "graphics/texture_manager.hpp"
#include <fstream>
#include <sstream>

namespace exd {
namespace graphics {

const uint32_t TextureManager::uploadToGPU(Texture& texture) {
    return -1;
}

const uint32_t TextureManager::uploadToGPU(components::CubeMap& cubemap) {
    auto it = textures_.find(cubemap.texture_handle);
    if (it != textures_.end()) return it->second.id;

    uint32_t nextId = textures_.size()+1;
    textures_.try_emplace(nextId, cubemap);
    return nextId;
}


const TextureGPU* TextureManager::bind(const uint32_t handle) {
    if (textures_.find(handle) == textures_.end()) {
        throw std::runtime_error("Texture handle doesn't exist");
    }

    // std::cout << "texture found at handle: " << handle << std::endl;
    TextureGPU* textureGPU = &textures_.at(handle);

    // std::cout << "got textureGPU* " << std::endl;
    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, textureGPU->id));
    // std::cout << "bound texture" << std::endl;
    return textureGPU;
}

} // namespace graphics
} // namespace exd
