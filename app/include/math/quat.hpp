#pragma once
#include "math/vec3.hpp"
#include <ostream>

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

     inline Quat norm() {
        float len2 = x*x + y*y + z*z + w*w;
        float inv  = 1.0f / std::sqrt(len2);
        return Quat{w*inv,x*inv,y*inv,z*inv};
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
