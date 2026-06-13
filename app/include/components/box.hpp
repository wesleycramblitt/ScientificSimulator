#pragma once
#include "icomponent.hpp"
#include "math/vec3.hpp"

namespace exd {
namespace components {

struct Box {
    math::Vec3 halfExtents;
};

} // namespace components
} // namespace exd
