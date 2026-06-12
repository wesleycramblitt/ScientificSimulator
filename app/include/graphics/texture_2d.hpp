#pragma once
#include "graphics/itexture_source.hpp"
#include <vector>
#include <cstdint>

namespace exd {
namespace graphics {

// Simple 2D texture from raw pixel data.
class Texture2D : public ITextureSource {
public:
    Texture2D(int w, int h, int channels, const unsigned char* pixels)
        : width_(w), height_(h), channels_(channels)
    {
        size_t size = (size_t)w * h * channels;
        data_.assign(pixels, pixels + size);
    }

    // ── ITextureSource ──
    GLenum gl_target()          const override { return GL_TEXTURE_2D; }
    GLenum gl_internal_format() const override { return channels_ == 4 ? GL_RGBA8 : GL_RGB8; }
    GLenum gl_format()          const override { return channels_ == 4 ? GL_RGBA  : GL_RGB;  }
    GLenum gl_pixel_type()      const override { return GL_UNSIGNED_BYTE; }
    int    width()              const override { return width_; }
    int    height()             const override { return height_; }
    int    depth()              const override { return 1; }

    bool upload_level(int level, int /*face_idx*/) const override {
        if (level != 0) return false;
        if (data_.empty()) return false;
        glTexImage2D(gl_target(), level, gl_internal_format(),
                     width_, height_, 0,
                     gl_format(), gl_pixel_type(), data_.data());
        return true;
    }

private:
    int width_, height_, channels_;
    std::vector<unsigned char> data_;
};

} // namespace graphics
} // namespace exd
