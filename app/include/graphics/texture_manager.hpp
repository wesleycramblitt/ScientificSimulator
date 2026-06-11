#pragma once
#include <string>
#include <unordered_map>
#include <glad/gl.h>
#include "graphics/texture.hpp"
#include "graphics/texture_gpu.hpp"
#include "components/cubemap.hpp"

namespace exd {
namespace graphics {

class TextureManager {
public:
    const uint32_t uploadToGPU(Texture& texture);
    const uint32_t uploadToGPU(components::CubeMap& cubemap);
    const TextureGPU* bind(const uint32_t handle);

private:
    std::unordered_map<std::uint32_t, TextureGPU> textures_;

};

} // namespace graphics
} // namespace exd
