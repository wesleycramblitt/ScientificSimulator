#include "systems/fluentx3d_system.hpp"

// TODO? wire fluidx3d to this system, use entities to create simulation
// Parameters for a minimum wind tunnel
// Grid:      nx=128, ny=256, nz=128   (streamwise=Y, longest)
// Viscosity: nu = 0.005
// Velocity:  D3Q19, SRT
// Extensions: EQUILIBRIUM_BOUNDARIES | SUBGRID | FORCE_FIELD
// STL:       assets/models/F117/F117.stl  (binary — already confirmed)
//            scale = 0.0  (auto-fill ~80% of domain)
//            center at (nx/2, ny/2, nz/2)
// Boundary flags (cell types from defines.hpp)
// Face	Flag	Meaning
// y == 0	TYPE_E (0x02)	Inlet
// y == ny-1	TYPE_E (0x02)	Outlet
// x==0, x==nx-1, z==0, z==nz-1	TYPE_S (0x01)	Wall
// Voxelized mesh cells	TYPE_S (0x01)	Solid
// Interior (no flag)	0	Fluid
// Initial velocity
// u.y = 0.05  (streamwise, must be < 0.577)
// Set only in non-TYPE_S cells
// API call order
// 1. fluidx3d_create(&cfg)
// 2. fluidx3d_voxelize_stl(...)          // marks TYPE_S from mesh
// 3. fluidx3d_read_field(FLAGS) → set TYPE_E on faces → fluidx3d_write_field(FLAGS)
// 4. fluidx3d_read_field(FLAGS) → fill u.y=0.05 where not TYPE_S → fluidx3d_write_field(U)
// 5. fluidx3d_initialize()
// 6. fluidx3d_run(solver, N, N)          // runs N steps
// 7. fluidx3d_export_vtk(U, "path.vtk")  // exports velocity field
FluentX3DSystem::FluentX3DSystem() {


}

FluentX3DSystem::start(Registry& registry, Window& window) {

}

FluentX3DSystem::update(Registry& registry,Window& window, float dt) {

 }
