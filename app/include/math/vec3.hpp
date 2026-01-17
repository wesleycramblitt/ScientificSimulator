#include <cmath>


struct Vec3 { float x, y, z; };
static Vec3 operator+(Vec3 a, Vec3 b){ return {a.x+b.x, a.y+b.y, a.z+b.z}; }
static Vec3 operator-(Vec3 a, Vec3 b){ return {a.x-b.x, a.y-b.y, a.z-b.z}; }
static Vec3 operator*(Vec3 a, float s){ return {a.x*s, a.y*s, a.z*s}; }

static float dot(Vec3 a, Vec3 b){ return a.x*b.x + a.y*b.y + a.z*b.z; }
static Vec3 cross(Vec3 a, Vec3 b){
  return { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
}
static Vec3 norm(Vec3 v){
  float len = std::sqrt(dot(v,v));
  if (len <= 0.00001f) return {0,0,0};
  return {v.x/len, v.y/len, v.z/len};
}
