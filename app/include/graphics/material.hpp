#pragma once
#include "graphics/texture.hpp"
#include "math/vec3.hpp"

enum ShaderTechnique {  MESHBASIC, PBR, RAYTRACING }; 

struct Material {
    Texture base_texture;
    Vec3 base_color;
    ShaderTechnique shader_technique; 
    //TODO: PBR materials, map textures, other attributes to mesh data 
    // std::array<Texture, (size_t)TexSlot::Count> tex{};

};


