#pragma once
#include "math/vec3.hpp"

namespace exd {
namespace components {

/// Stores shear/skew factors applied in the model matrix as T * R * K * S.
/// The three components represent:
///   shear.x = XY shear (displaces X proportionally to Y)
///   shear.y = XZ shear (displaces X proportionally to Z)
///   shear.z = YZ shear (displaces Y proportionally to Z)
struct Skew {
    math::Vec3 shear{0.0f, 0.0f, 0.0f};
};

} // namespace components
} // namespace exd
