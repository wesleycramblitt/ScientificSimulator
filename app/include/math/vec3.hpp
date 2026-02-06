#pragma once
#include <cmath>


struct Vec3 { float x, y, z;

    Vec3 norm(){
      float len = std::sqrt(x*x + y*y + z*z);
      if (len <= 0.00001f) return {0,0,0};
      return {x/len, y/len, z/len};
    }

        
    float dot(Vec3 b){ return x*b.x + y*b.y + z*b.z; }
    
    Vec3 cross(Vec3 b){
      return { y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x };
    }

};

static Vec3 operator+(Vec3 a, Vec3 b){ return {a.x+b.x, a.y+b.y, a.z+b.z}; }
static Vec3 operator-(Vec3 a, Vec3 b){ return {a.x-b.x, a.y-b.y, a.z-b.z}; }
static Vec3 operator*(Vec3 a, float s){ return {a.x*s, a.y*s, a.z*s}; }

inline Vec3 operator*(float s, const Vec3& v)
{
    return { s * v.x, s * v.y, s * v.z };
}

