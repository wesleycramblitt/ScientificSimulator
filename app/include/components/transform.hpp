#include "math/vec3.hpp"
#include "math/quat.hpp"

struct Transform {
    Vec3 position;
    Quat rotation;
    Vec3 scale(1,1,1);
};
