#pragma once
#include <glad/gl.h>
#include <string>
#include <utility>

namespace exd {
namespace graphics {

struct TextureGPU {
    GLuint id     = 0;
    GLenum target = 0;
    std::string name;

    TextureGPU() = default;
    TextureGPU(GLuint _id, GLenum _target) : id(_id), target(_target) {}

    TextureGPU(const TextureGPU&) = delete;
    TextureGPU& operator=(const TextureGPU&) = delete;

    TextureGPU(TextureGPU&& other) noexcept
        : id(std::exchange(other.id, 0)),
          target(std::exchange(other.target, 0)),
          name(std::move(other.name)) {}

    TextureGPU& operator=(TextureGPU&& other) noexcept {
        if (this != &other) {
            if (id) glDeleteTextures(1, &id);
            id     = std::exchange(other.id, 0);
            target = std::exchange(other.target, 0);
            name   = std::move(other.name);
        }
        return *this;
    }

    ~TextureGPU() {
        if (id) glDeleteTextures(1, &id);
    }
};

} // namespace graphics
} // namespace exd
