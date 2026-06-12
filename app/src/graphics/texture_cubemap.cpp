#include "graphics/texture_cubemap.hpp"
#include "common/macros.hpp"
#include "stb_image.h"
#include <vector>
#include <cstring>

namespace exd {
namespace graphics {

// GL cubemap face targets in +X, -X, +Y, -Y, +Z, -Z order
static const GLenum kCubemapFaces[6] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};

bool CubeMapTexture::upload_level(int level, int face_idx) const {
    if (level != 0) return false;

    if (cross_layout_) {
        // Single cross-shaped image — split into 6 faces on first call
        // We upload all 6 faces when face_idx == 0 and skip on 1..5.
        if (face_idx != 0) return false;

        int imgW, imgH, channels;
        unsigned char* src = stbi_load(cross_path_.c_str(),
                                       &imgW, &imgH, &channels, 0);
        if (!src) return false;

        int faceW = imgW / 4;
        int faceH = imgH / 3;
        GLenum fmt = (channels == 4) ? GL_RGBA : GL_RGB;

        // Face offsets in a standard vertical-cross layout:
        //        [ +Y ]
        // [ -X ] [ +Z ] [ +X ] [ -Z ]
        //        [ -Y ]
        struct { int col, row; } offsets[6] = {
            {2, 1},  // +X
            {0, 1},  // -X
            {1, 0},  // +Y
            {1, 2},  // -Y
            {1, 1},  // +Z
            {3, 1},  // -Z
        };

        std::vector<unsigned char> faceData(faceW * faceH * channels);
        for (int f = 0; f < 6; ++f) {
            int ox = offsets[f].col * faceW;
            int oy = offsets[f].row * faceH;
            for (int y = 0; y < faceH; ++y) {
                const unsigned char* srcRow = src + ((oy + y) * imgW + ox) * channels;
                unsigned char* dstRow = faceData.data() + y * faceW * channels;
                std::memcpy(dstRow, srcRow, faceW * channels);
            }
            glTexImage2D(kCubemapFaces[f], 0, fmt,
                         faceW, faceH, 0, fmt, GL_UNSIGNED_BYTE, faceData.data());
        }
        stbi_image_free(src);
        return true;

    } else {
        // Individual face file
        if (face_idx < 0 || face_idx > 5) return false;

        const auto& path = face_paths_[face_idx];
        int w, h, ch;
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);
        if (!data) return false;

        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(kCubemapFaces[face_idx], 0, fmt,
                     w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
        return true;
    }
}

} // namespace graphics
} // namespace exd
