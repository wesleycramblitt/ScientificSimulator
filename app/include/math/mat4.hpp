#include "math/vec3.hpp"

struct Mat4 { float m[16]; }; // column-major

static Mat4 identity() {
  Mat4 r{};
  r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
  return r;
}

static Mat4 mul(const Mat4& a, const Mat4& b) {
  Mat4 r{};
  for (int c = 0; c < 4; c++){
    for (int r0 = 0; r0 < 4; r0++){
      r.m[c*4 + r0] =
        a.m[0*4 + r0]*b.m[c*4 + 0] +
        a.m[1*4 + r0]*b.m[c*4 + 1] +
        a.m[2*4 + r0]*b.m[c*4 + 2] +
        a.m[3*4 + r0]*b.m[c*4 + 3];
    }
  }
  return r;
}

static Mat4 perspective(float fovy_rad, float aspect, float znear, float zfar) {
  float f = 1.0f / std::tan(fovy_rad * 0.5f);
  Mat4 r{};
  r.m[0]  = f / aspect;
  r.m[5]  = f;
  r.m[10] = (zfar + znear) / (znear - zfar);
  r.m[11] = -1.0f;
  r.m[14] = (2.0f * zfar * znear) / (znear - zfar);
  return r;
}

static Mat4 lookat(Vec3 eye, Vec3 center, Vec3 up) {
  Vec3 f = norm(center - eye);
  Vec3 s = norm(cross(f, up));
  Vec3 u = cross(s, f);

  mat4 r = identity();
  r.m[0]  = s.x;  r.m[4]  = s.y;  r.m[8]  = s.z;
  r.m[1]  = u.x;  r.m[5]  = u.y;  r.m[9]  = u.z;
  r.m[2]  = -f.x; r.m[6]  = -f.y; r.m[10] = -f.z;

  r.m[12] = -dot(s, eye);
  r.m[13] = -dot(u, eye);
  r.m[14] =  dot(f, eye);
  return r;
}

