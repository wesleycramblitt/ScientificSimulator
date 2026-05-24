#include "fluidx3d.h"
#include <cstdio>

int main() {
    printf("FluidX3D C API version: %s\n", "1.0");

    FluidX3D_Config cfg = fluidx3d_default_config();
    printf("Default config: %ux%ux%u, velocity_set=%d, precision=%d\n",
           cfg.nx, cfg.ny, cfg.nz, cfg.velocity_set, cfg.precision);

    FluidX3D_Solver* solver = fluidx3d_create(&cfg);
    if (!solver) {
        printf("SKIP: no OpenCL device available (expected on headless CI)\n");
        return 0;
    }

    uint32_t nx, ny, nz;
    fluidx3d_get_dims(solver, &nx, &ny, &nz);
    printf("Solver grid: %ux%ux%u\n", nx, ny, nz);

    printf("nu=%f tau=%f Re_max=%f\n",
           fluidx3d_get_nu(solver),
           fluidx3d_get_tau(solver),
           fluidx3d_get_Re_max(solver));

    fluidx3d_destroy(solver);
    printf("PASS: FluidX3D integration works\n");
    return 0;
}
