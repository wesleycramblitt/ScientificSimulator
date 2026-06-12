#pragma once
#include "graphics/itexture_source.hpp"
#include <string>
#include <array>
#include <stdexcept>

namespace exd {
namespace graphics {

// Cubemap texture source. Supports two layouts:
// 1. Six individual face files  (+X, -X, +Y, -Y, +Z, -Z)
// 2. A single cross-shaped image (cross_layout)
class CubeMapTexture : public ITextureSource {
public:
    /// Construct from six individual face paths.
    CubeMapTexture(std::array<std::string, 6> face_paths)
        : cross_layout_(false), face_paths_(std::move(face_paths)) {}

    /// Construct from a single cross-shaped image.
    CubeMapTexture(const std::string& cross_path, int face_size)
        : cross_layout_(true), cross_path_(cross_path), face_size_(face_size) {}

    // ── ITextureSource ──
    GLenum gl_target()          const override { return GL_TEXTURE_CUBE_MAP; }
    GLenum gl_internal_format() const override { return GL_RGB8; }
    GLenum gl_format()          const override { return GL_RGB;  }
    GLenum gl_pixel_type()      const override { return GL_UNSIGNED_BYTE; }
    int    width()              const override { return face_size_ > 0 ? face_size_ : 512; }
    int    height()             const override { return face_size_ > 0 ? face_size_ : 512; }
    int    depth()              const override { return 1; }

    bool upload_level(int level, int face_idx) const override;

private:
    bool cross_layout_;
    std::array<std::string, 6> face_paths_;
    std::string cross_path_;
    int face_size_ = 0;
};

} // namespace graphics
} // namespace exd
