// Separate file for createDomainBox to avoid Mesh name collision
// between graphics/mesh.hpp and FluidX3D's utilities.hpp Mesh.
#include "systems/fluidx3d_system.hpp"
#include "components/fluid_domain.hpp"
#include "components/transform.hpp"
#include "graphics/mesh.hpp"
#include "graphics/vertex.hpp"
#include "math/vec3.hpp"

Mesh FluidX3DSystem::createDomainBox(const SimulationDomain& domain, const Transform& /*xform*/) {
    Mesh mesh;
    mesh.topology = LINES;
    const int nx = domain.nx, ny = domain.ny, nz = domain.nz;
    const float hx = (float)nx * 0.5f, hy = (float)ny * 0.5f, hz = (float)nz * 0.5f;
    const Vec3 gridColor{0.25f, 0.65f, 0.65f};
    const Vec3 edgeColor{0.5f, 1.0f, 1.0f};

    auto line = [&](float x1,float y1,float z1, float x2,float y2,float z2, const Vec3& c) {
        mesh.vertices.push_back({Vec3{x1,y1,z1}, c});
        mesh.vertices.push_back({Vec3{x2,y2,z2}, c});
    };

    const int div = 10;
    const float dx = 2.0f * hx / (float)div;
    const float dy = 2.0f * hy / (float)div;
    const float dz = 2.0f * hz / (float)div;

    for (int f = 0; f < 2; ++f) {
        const float y = f ? hy : -hy;
        for (int i = 0; i <= div; ++i) {
            float x = -hx + (float)i * dx;
            line(x, y, -hz, x, y, hz, gridColor);
            float z = -hz + (float)i * dz;
            line(-hx, y, z, hx, y, z, gridColor);
        }
    }
    for (int f = 0; f < 2; ++f) {
        const float z = f ? hz : -hz;
        for (int i = 0; i <= div; ++i) {
            float x = -hx + (float)i * dx;
            line(x, -hy, z, x, hy, z, gridColor);
            float y = -hy + (float)i * dy;
            line(-hx, y, z, hx, y, z, gridColor);
        }
    }
    for (int f = 0; f < 2; ++f) {
        const float x = f ? hx : -hx;
        for (int i = 0; i <= div; ++i) {
            float y = -hy + (float)i * dy;
            line(x, y, -hz, x, y, hz, gridColor);
            float z = -hz + (float)i * dz;
            line(x, -hy, z, x, hy, z, gridColor);
        }
    }

    // Outer edges
    line(-hx, -hy, -hz,  hx, -hy, -hz, edgeColor);
    line(-hx, -hy,  hz,  hx, -hy,  hz, edgeColor);
    line(-hx, -hy, -hz, -hx, -hy,  hz, edgeColor);
    line( hx, -hy, -hz,  hx, -hy,  hz, edgeColor);
    line(-hx,  hy, -hz,  hx,  hy, -hz, edgeColor);
    line(-hx,  hy,  hz,  hx,  hy,  hz, edgeColor);
    line(-hx,  hy, -hz, -hx,  hy,  hz, edgeColor);
    line( hx,  hy, -hz,  hx,  hy,  hz, edgeColor);
    line(-hx, -hy, -hz, -hx,  hy, -hz, edgeColor);
    line( hx, -hy, -hz,  hx,  hy, -hz, edgeColor);
    line(-hx, -hy,  hz, -hx,  hy,  hz, edgeColor);
    line( hx, -hy,  hz,  hx,  hy,  hz, edgeColor);

    return mesh;
}
