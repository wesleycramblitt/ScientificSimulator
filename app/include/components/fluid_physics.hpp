#pragma once
#include<cstdint>

namespace exd {
namespace components {

struct FluidPhysics {
    float nu; //viscosity
    float streamwise_velocity; //inlet speed
    uint8_t streamwise_axis; // 0=X 1=Y 2=Z fluid direction
    float fx,fy,fz; // volume force (gravity, pressure gradient)
    float sigma; // surface tnesion (0 = single phase)
};

} // namespace components
} // namespace exd
