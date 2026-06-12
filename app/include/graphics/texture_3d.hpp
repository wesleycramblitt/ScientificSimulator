#pragma once
#include "graphics/itexture_source.hpp"
#include <cstdio>

namespace exd {
namespace graphics {

// 3D volume texture for scalar-field rendering (e.g. velocity magnitude).
// Holds a pointer to caller-owned data. First uploadToGPU call does
// glTexImage3D; subsequent update() calls do glTexSubImage3D.
class Texture3D : public ITextureSource {
public:
    Texture3D(int nx, int ny, int nz, const float* data = nullptr)
        : nx_(nx), ny_(ny), nz_(nz), data_(data) {}

    // Update the data pointer for subsequent frames (caller-owned memory).
    void set_data(const float* data) { data_ = data; }

    // ── ITextureSource ──
    GLenum gl_target()          const override { return GL_TEXTURE_3D; }
    GLenum gl_internal_format() const override { return GL_R32F; }
    GLenum gl_format()          const override { return GL_RED;  }
    GLenum gl_pixel_type()      const override { return GL_FLOAT; }
    int    width()              const override { return nx_; }
    int    height()             const override { return ny_; }
    int    depth()              const override { return nz_; }

    GLenum min_filter() const override { return GL_LINEAR;          }
    GLenum mag_filter() const override { return GL_LINEAR;          }
    GLenum wrap_s()     const override { return GL_CLAMP_TO_BORDER; }
    GLenum wrap_t()     const override { return GL_CLAMP_TO_BORDER; }
    GLenum wrap_r()     const override { return GL_CLAMP_TO_BORDER; }

    bool upload_level(int level, int /*face_idx*/) const override {
        if (level != 0) return false;
        if (!data_) return false;
        glTexImage3D(GL_TEXTURE_3D, level, gl_internal_format(),
                     nx_, ny_, nz_, 0,
                     gl_format(), gl_pixel_type(), data_);
        printf("[Texture3D] Created %dx%dx%d R32F\n", nx_, ny_, nz_);
        return true;
    }

    // Per-frame update via glTexSubImage3D
    void update_level(int /*level*/, int /*face_idx*/) const override {
        if (!data_) return;
        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx_, ny_, nz_,
                        gl_format(), gl_pixel_type(), data_);
    }

private:
    int nx_, ny_, nz_;
    const float* data_ = nullptr;
};

} // namespace graphics
} // namespace exd
