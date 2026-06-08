#include "math/vec3.hpp"
#include "math/quat.hpp"

struct Transform {
    Vec3 position{0,0,0};
    Quat rotation{1, 0,0,0};
    Vec3 scale { 1,1,1};
};
