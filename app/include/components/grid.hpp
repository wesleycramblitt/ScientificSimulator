#pragma once
#include "icomponent.hpp"
#include "math/vec3.hpp"
#include "math/quat.hpp"

namespace exd {
namespace components {

struct Grid {
    float spacing;
    math::Quat color{0.5,0.5,0.5,1.0};
};

} // namespace components
} // namespace exd
