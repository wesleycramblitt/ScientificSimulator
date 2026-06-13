#pragma once
#include "icomponent.hpp"
#include "math/vec3.hpp"

namespace exd {
namespace components {

struct Render_Technique_Lit {
    math::Vec3  light_dir{0.0f, -0.866f, -0.3f};
};

} // namespace components
} // namespace exd
