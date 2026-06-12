#pragma once
#include <string>
#include <unordered_map>
#include <glad/gl.h>
#include "graphics/itexture_source.hpp"
#include "graphics/texture_gpu.hpp"

namespace exd {
namespace graphics {

class TextureManager {
public:
    /// Upload any ITextureSource to the GPU. Returns a handle for later bind/update.
    uint32_t uploadToGPU(ITextureSource& source);

    /// Update an existing 3D texture with new data (glTexSubImage3D).
    /// The source must be the same Texture3D originally uploaded.
    void update(uint32_t handle, ITextureSource& source);

    /// Bind a texture to the given texture unit. Returns the TextureGPU.
    const TextureGPU* bind(uint32_t handle, GLenum texture_unit = GL_TEXTURE0);

    /// Remove from the manager and delete the GL object.
    void destroy(uint32_t handle);

private:
    std::unordered_map<uint32_t, TextureGPU> textures_;
    uint32_t next_handle_ = 1;
};

} // namespace graphics
} // namespace exd
