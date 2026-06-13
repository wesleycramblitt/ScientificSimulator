#pragma once
#include "icomponent.hpp"
#include "math/vec3.hpp"
#include "math/quat.hpp"

namespace exd {
namespace components {

struct Transform {
    math::Vec3 position{0,0,0};
    math::Quat rotation{1, 0,0,0};
    math::Vec3 scale { 1,1,1};
};

} // namespace components
} // namespace exd
