#pragma once
#include "graphics/texture.hpp"
#include "graphics/mesh.hpp"
#include "math/vec3.hpp"

struct Material {
    Texture texture;
    Vec3 rgb;
    Mesh mesh;
};


