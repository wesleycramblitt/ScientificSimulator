#pragma once
#include "math/vec3.hpp"
#include <ostream>

namespace exd {
namespace math {

struct Quat {
    
    float w,x,y,z;
    
    inline Quat operator*(const Quat& b) const
    {
        return Quat{
            w*b.w - x*b.x - y*b.y - z*b.z,
            w*b.x + x*b.w + y*b.z - z*b.y,
            w*b.y - x*b.z + y*b.w + z*b.x,
            w*b.z + x*b.y - y*b.x + z*b.w
        };
    }

   
    static Quat fromAxisAngle(Vec3& axis, float angleRad)
    {
        Vec3 n = axis.norm();           // ensure unit axis
        float half = 0.5f * angleRad;

        float s = std::sin(half);
        float c = std::cos(half);

        return Quat{
            c,
            n.x * s,
            n.y * s,
            n.z * s
        };
    }

    inline Vec3 right() 
    {
        return Vec3{
            1 - 2*(y*y + z*z),
            2*(x*y + w*z),
            2*(x*z - w*y)
        };
    }

    inline Vec3 forward() {
        
        return {
            (2.0f * (x*z + w*y)),
            (2.0f * (y*z - w*x)),
            -(1.0f - 2.0f * (x*x + y*y))
        };
    }

    inline Vec3 up() {
        return {
        2.0f * (x*y - w*z),
        1.0f - 2.0f * (x*x + z*z),
        2.0f * (y*z + w*x)
    };
    }

      inline Quat norm() const {
        float len2 = x*x + y*y + z*z + w*w;
        float inv  = 1.0f / std::sqrt(len2);
        return Quat{w*inv,x*inv,y*inv,z*inv};
    }

    // Extract quaternion from a pure rotation matrix (column-major, upper-left 3x3).
    // Assumes the matrix is orthonormal (no scale or shear).
    static Quat fromRotationMatrix(const float* m) {
        // m is column-major: m[col*4 + row]
        const float r00 = m[0],  r01 = m[4],  r02 = m[8];
        const float r10 = m[1],  r11 = m[5],  r12 = m[9];
        const float r20 = m[2],  r21 = m[6],  r22 = m[10];

        float w, x, y, z;
        const float trace = r00 + r11 + r22;

        if (trace > 0.0f) {
            float s = std::sqrt(trace + 1.0f) * 2.0f;
            w = 0.25f * s;
            x = (r21 - r12) / s;
            y = (r02 - r20) / s;
            z = (r10 - r01) / s;
        } else if (r00 > r11 && r00 > r22) {
            float s = std::sqrt(1.0f + r00 - r11 - r22) * 2.0f;
            w = (r21 - r12) / s;
            x = 0.25f * s;
            y = (r01 + r10) / s;
            z = (r02 + r20) / s;
        } else if (r11 > r22) {
            float s = std::sqrt(1.0f + r11 - r00 - r22) * 2.0f;
            w = (r02 - r20) / s;
            x = (r01 + r10) / s;
            y = 0.25f * s;
            z = (r12 + r21) / s;
        } else {
            float s = std::sqrt(1.0f + r22 - r00 - r11) * 2.0f;
            w = (r10 - r01) / s;
            x = (r02 + r20) / s;
            y = (r12 + r21) / s;
            z = 0.25f * s;
        }

        return Quat{w, x, y, z};
    }


};


inline std::ostream& operator<<(std::ostream& os, const Quat& q)
{
    os << "Quat("
       << "w=" << q.w << ", "
       << "x=" << q.x << ", "
       << "y=" << q.y << ", "
       << "z=" << q.z << ")";
    return os;
}

inline Vec3 operator*(const Quat& q, const Vec3& v)
{
    Vec3 qv{ q.x, q.y, q.z };

    Vec3 t = 2.0f * qv.cross(v);
    Vec3 result = v + q.w * t + qv.cross(t);

    return result;
}

} // namespace math
} // namespace exd
