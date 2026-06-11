#pragma once
#include <string>
#include <cstdint>
#include <cstring>
#include <vector>
#include <glad/gl.h>
#include "common/macros.hpp"
#include "components/cubemap.hpp"
#include <stdexcept>
#include "stb_image.h"

namespace exd {
namespace graphics {

struct TextureGPU {
    TextureGPU(components::CubeMap cubemap) {
        glGenTextures(1, &id);
        GL_CALL(glActiveTexture(GL_TEXTURE0));
        GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, id));

        if (cubemap.cross_layout) {
            // Single cross-shaped image containing all 6 faces
            int imgW, imgH, channels;
            unsigned char* src = stbi_load(cubemap.faces[0].name.c_str(),
                                           &imgW, &imgH, &channels, 0);
            if (!src) {
                glDeleteTextures(1, &id);
                throw std::runtime_error("Failed to load cross cubemap: " + cubemap.faces[0].name);
            }

            int faceW = imgW / 4;
            int faceH = imgH / 3;
            GLenum fmt = (channels == 4) ? GL_RGBA : GL_RGB;

            // Face offsets in the cross layout:  { GL face, col, row }
            struct { GLenum target; int col, row; } faces[6] = {
                { GL_TEXTURE_CUBE_MAP_POSITIVE_X, 2, 1 },
                { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, 1 },
                { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 1, 0 },
                { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 1, 2 },
                { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 1, 1 },
                { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 3, 1 },
            };

            std::vector<unsigned char> faceData(faceW * faceH * channels);
            for (int f = 0; f < 6; ++f) {
                int ox = faces[f].col * faceW;
                int oy = faces[f].row * faceH;
                for (int y = 0; y < faceH; ++y) {
                    const unsigned char* srcRow = src + ((oy + y) * imgW + ox) * channels;
                    unsigned char* dstRow = faceData.data() + y * faceW * channels;
                    memcpy(dstRow, srcRow, faceW * channels);
                }
                glTexImage2D(faces[f].target, 0, fmt,
                             faceW, faceH, 0, fmt, GL_UNSIGNED_BYTE, faceData.data());
            }
            stbi_image_free(src);

        } else {
            // Six individual face files
            for (size_t i = 0; i < 6; ++i) {
                unsigned char* data = stbi_load(cubemap.faces[i].name.c_str(),
                                                &cubemap.faces[i].width,
                                                &cubemap.faces[i].height,
                                                &cubemap.faces[i].channels,
                                                0);
                if (!data) {
                    glDeleteTextures(1, &id);
                    throw std::runtime_error("Failed to load cubemap face: " + cubemap.faces[i].name);
                }

                GLenum format = (cubemap.faces[i].channels == 4) ? GL_RGBA : GL_RGB;
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + (GLenum)i,
                             0, format,
                             cubemap.faces[i].width,
                             cubemap.faces[i].height,
                             0, format, GL_UNSIGNED_BYTE, data);

                stbi_image_free(data);
            }
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));

    };
    TextureGPU(const TextureGPU&) = delete;
    TextureGPU& operator=(const TextureGPU&) = delete;

    TextureGPU(TextureGPU&& other) noexcept : id(other.id) {
        other.id = 0;
    }
    TextureGPU& operator=(TextureGPU&& other) noexcept {
        if (this != &other) {
            if (id) glDeleteTextures(1, &id);
            id = other.id;
            other.id = 0;
        }
        return *this;
    }
    ~TextureGPU() {
        glDeleteTextures(1, &id);
        id = 0;
    };
    std::string name;
    GLuint id;

};

} // namespace graphics
} // namespace exd
