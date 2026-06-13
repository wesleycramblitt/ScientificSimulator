#pragma once
#include "icomponent.hpp"
#include <cstdint>

namespace exd {
namespace components {

struct Renderable {
    //TODO: multiple meshes and materials grouped?
    //std::uint32_t material;
    std::uint32_t    mesh;
};

} // namespace components
} // namespace exd
