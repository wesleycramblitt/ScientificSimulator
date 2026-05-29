#include "lbm.hpp"
#include "defines.hpp"
#include <cstdio>

int main() {
    const uint nx = 192, ny = 64, nz = 64;
    const float nu = 0.005f;
    const float fy = 5.43e-8f;

    printf("Creating LBM %ux%ux%u nu=%.4f fy=%.2e...\n", nx, ny, nz, nu, fy);
    LBM* lbm = new LBM(nx, ny, nz, nu, 0.0f, fy, 0.0f);
    printf("LBM created.\n");

    // Set walls
    for (uint z = 0; z < nz; z++)
        for (uint y = 0; y < ny; y++)
            for (uint x = 0; x < nx; x++) {
                if (x == 0 || x == nx-1 || z == 0 || z == nz-1) {
                    ulong i = (ulong)x + ((ulong)y + (ulong)z * ny) * nx;
                    lbm->flags[i] = TYPE_S;
                }
            }
    printf("Walls set.\n");

    printf("Voxelizing...\n");
    lbm->voxelize_stl("assets/models/F117/F117.stl",
                       float3(nx*0.5f, ny*0.5f, nz*0.5f), 0.3f, TYPE_S);
    printf("Voxelized.\n");

    lbm->run(10);
    printf("Ran 10 steps.\n");

    delete lbm;
    printf("Done.\n");
    return 0;
}
