#pragma once
#include <string>
#include <cstdint>
#include <glad/gl.h>
#include "common/macros.hpp"

namespace exd {
namespace graphics {

struct Texture {
    std::string name;
    int width;
    int height;
    int depth = 1;
    int channels; 
    int mipLevels = 1;
};

} // namespace graphics
} // namespace exd

