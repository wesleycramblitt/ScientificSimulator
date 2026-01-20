#pragma once
#include <cstdint>
#include "math/vec3.hpp"
#include "math/quat.hpp"
#include "graphics/mesh.hpp"

enum class ColliderShape : uint8_t {
    Box,
    Sphere,
    Capsule,
    Plane,
    Mesh
};

struct ColliderMaterial {
    float friction    = 0.6f;
    float restitution = 0.1f;
};

struct Collider {
    ColliderShape shape = ColliderShape::Box;

    // Local pose relative to entity Transform (supports offset colliders)
    Vec3 local_offset {0,0,0};
    Quat local_rotation; // identity by default

    // Shape parameters (interpret based on shape)
    Vec3 half_extents {0.5f, 0.5f, 0.5f}; // Box
    float radius = 0.5f;                 // Sphere/Capsule
    float half_height = 0.5f;            // Capsule (height excluding hemispheres)

    // Plane: normal is local +Y after applying local_rotation; distance via local_offset

    // Mesh collider (typically static only in v1)
    Mesh mesh = 0;                 // only used if shape == Mesh

    ColliderMaterial material {};

    bool is_trigger = false;             // for overlap events; no resolution
};
