#pragma once
#include <glad/gl.h>

namespace exd {
namespace graphics {

// Abstract interface for any texture source the TextureManager can upload.
// Implementations: Texture2D, Texture3D, CubeMapTexture.
class ITextureSource {
public:
    virtual ~ITextureSource() = default;

    // ── GL parameters ──
    virtual GLenum gl_target()          const = 0;  // GL_TEXTURE_2D, _3D, _CUBE_MAP
    virtual GLenum gl_internal_format() const = 0;  // GL_RGB8, GL_RGBA8, GL_R32F, ...
    virtual GLenum gl_format()          const = 0;  // GL_RGB, GL_RGBA, GL_RED, ...
    virtual GLenum gl_pixel_type()      const = 0;  // GL_UNSIGNED_BYTE, GL_FLOAT, ...

    // ── Dimensions ──
    virtual int width()  const = 0;
    virtual int height() const = 0;
    virtual int depth()  const = 0;     // 1 for 2D / cubemap

    // ── Upload hook ──
    // Called once per face × mip-level by TextureManager::uploadToGPU.
    // For 2D/3D, face_idx == 0 and this is called once.
    // For cubemaps, called 6 times with face_idx 0..5.
    // level = mip level (0 = base). Returns false if no data for this level.
    virtual bool upload_level(int level, int face_idx) const = 0;

    // ── Sampling parameters (optional overrides) ──
    virtual GLenum min_filter()     const { return GL_LINEAR;          }
    virtual GLenum mag_filter()     const { return GL_LINEAR;          }
    virtual GLenum wrap_s()         const { return GL_CLAMP_TO_EDGE;   }
    virtual GLenum wrap_t()         const { return GL_CLAMP_TO_EDGE;   }
    virtual GLenum wrap_r()         const { return GL_CLAMP_TO_EDGE;   }
    virtual int    max_mip_levels() const { return 1;                  }

    // ── 3D update (optional) ──
    // Called on subsequent frames to push new data to an existing 3D texture.
    // Default no-op. Only Texture3D overrides this.
    virtual void update_level(int /*level*/, int /*face_idx*/) const {}
};

} // namespace graphics
} // namespace exd
