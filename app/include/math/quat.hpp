#pragma once
#include "math/vec3.hpp"

struct Quat {
    float w,x,y,z;

    inline Vec3 forward() {
        return {
            2.0f * (x*z + w*y),
            2.0f * (y*z - w*x),
            -(1.0f - 2.0f * (x*x + y*y))
        };
    }

    inline Vec3 up() {
        return {
        2.0f * (x*y - w*z),
        1.0f - 2.0f * (x*x + z*z),
        2.0f * (y*z + w*x)
    };
    }
};
