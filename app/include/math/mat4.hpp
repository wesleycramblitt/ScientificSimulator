#pragma once
#include "math/vec3.hpp"
#include "math/quat.hpp"
#include <iostream>

namespace exd {
namespace math {

struct Mat4 { 
    float m[16]; 

    static Mat4 identity() {
      Mat4 r{};
      r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
      return r;
    }

    void print() {
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                std::cout << m[col * 4 + row] << " ";
            }
        std::cout << "\n";
    }
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


    static Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
      Vec3 f = (center - eye).norm();
      Vec3 s = f.cross(up).norm();
      Vec3 u = s.cross(f).norm();

      Mat4 r = identity();
      r.m[0]  = s.x;  r.m[4]  = s.y;  r.m[8]  = s.z;
      r.m[1]  = u.x;  r.m[5]  = u.y;  r.m[9]  = u.z;
      r.m[2]  = -f.x; r.m[6]  = -f.y; r.m[10] = -f.z;

      r.m[12] = -s.dot(eye);
      r.m[13] = -u.dot(eye);
      r.m[14] =  f.dot(eye);
      return r;
    }

    static Mat4 modelTRS(const Mat4& T, const Mat4& R, const Mat4& S) {
        return mul(mul(T, R), S);
    }

    static Mat4 modelTRS(const Vec3& p, Quat q, const Vec3& s) {
        q = q.norm();

        const float x = q.x, y = q.y, z = q.z, w = q.w;

        const float xx = x*x, yy = y*y, zz = z*z;
        const float xy = x*y, xz = x*z, yz = y*z;
        const float wx = w*x, wy = w*y, wz = w*z;

        // Rotation matrix entries (row, col)
        const float r00 = 1.0f - 2.0f*(yy + zz);
        const float r01 = 2.0f*(xy - wz);
        const float r02 = 2.0f*(xz + wy);

        const float r10 = 2.0f*(xy + wz);
        const float r11 = 1.0f - 2.0f*(xx + zz);
        const float r12 = 2.0f*(yz - wx);

        const float r20 = 2.0f*(xz - wy);
        const float r21 = 2.0f*(yz + wx);
        const float r22 = 1.0f - 2.0f*(xx + yy);

        Mat4 M{};

        // Column 0 = R.col0 * sx
        M.m[0] = r00 * s.x;  M.m[1] = r10 * s.x;  M.m[2]  = r20 * s.x;  M.m[3]  = 0.0f;
        // Column 1 = R.col1 * sy
        M.m[4] = r01 * s.y;  M.m[5] = r11 * s.y;  M.m[6]  = r21 * s.y;  M.m[7]  = 0.0f;
        // Column 2 = R.col2 * sz
        M.m[8] = r02 * s.z;  M.m[9] = r12 * s.z;  M.m[10] = r22 * s.z;  M.m[11] = 0.0f;

        // Column 3 = translation
        M.m[12] = p.x;       M.m[13] = p.y;       M.m[14] = p.z;       M.m[15] = 1.0f;

        return M;
    }

    // Build model matrix with skew: T * R * K * S
    // where K is the shear matrix built from Vec3 skew factors:
    //   skew.x = XY shear, skew.y = XZ shear, skew.z = YZ shear
    static Mat4 modelTRS(const Vec3& p, Quat q, const Vec3& s, const Vec3& skew) {
        q = q.norm();

        const float x = q.x, y = q.y, z = q.z, w = q.w;

        const float xx = x*x, yy = y*y, zz = z*z;
        const float xy = x*y, xz = x*z, yz = y*z;
        const float wx = w*x, wy = w*y, wz = w*z;

        const float r00 = 1.0f - 2.0f*(yy + zz);
        const float r01 = 2.0f*(xy - wz);
        const float r02 = 2.0f*(xz + wy);

        const float r10 = 2.0f*(xy + wz);
        const float r11 = 1.0f - 2.0f*(xx + zz);
        const float r12 = 2.0f*(yz - wx);

        const float r20 = 2.0f*(xz - wy);
        const float r21 = 2.0f*(yz + wx);
        const float r22 = 1.0f - 2.0f*(xx + yy);

        // Shear factors
        const float kxy = skew.x;  // shear X along Y
        const float kxz = skew.y;  // shear X along Z
        const float kyz = skew.z;  // shear Y along Z

        // Compute R * K first, then apply scale: (R*K) * S
        // Column 0 of R*K:
        //   r00*1 + r01*0 + r02*0 = r00
        //   r10*1 + r11*0 + r12*0 = r10
        //   r20*1 + r21*0 + r22*0 = r20
        // Column 1 of R*K:
        //   r00*kxy + r01*1 + r02*0
        //   r10*kxy + r11*1 + r12*0
        //   r20*kxy + r21*1 + r22*0
        // Column 2 of R*K:
        //   r00*kxz + r01*kyz + r02*1
        //   r10*kxz + r11*kyz + r12*1
        //   r20*kxz + r21*kyz + r22*1

        Mat4 M{};

        // Column 0 = (R*K).col0 * sx
        M.m[0] = r00 * s.x;
        M.m[1] = r10 * s.x;
        M.m[2] = r20 * s.x;
        M.m[3] = 0.0f;

        // Column 1 = (R*K).col1 * sy
        M.m[4] = (r00 * kxy + r01) * s.y;
        M.m[5] = (r10 * kxy + r11) * s.y;
        M.m[6] = (r20 * kxy + r21) * s.y;
        M.m[7] = 0.0f;

        // Column 2 = (R*K).col2 * sz
        M.m[8]  = (r00 * kxz + r01 * kyz + r02) * s.z;
        M.m[9]  = (r10 * kxz + r11 * kyz + r12) * s.z;
        M.m[10] = (r20 * kxz + r21 * kyz + r22) * s.z;
        M.m[11] = 0.0f;

        // Column 3 = translation
        M.m[12] = p.x;
        M.m[13] = p.y;
        M.m[14] = p.z;
        M.m[15] = 1.0f;

        return M;
    }
};

} // namespace math
} // namespace exd
