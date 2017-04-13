#ifndef FMO_ALGEBRA_HPP
#define FMO_ALGEBRA_HPP

#include <fmo/common.hpp>
#include <math.h>

namespace fmo {
    /// Vector in 2D euclidean coordinates.
    struct Vector {
        constexpr Vector(int aX, int aY) : x(aX), y(aY) {}

        Vector(const Vector&) = default;
        Vector& operator=(const Vector&) = default;

        // data
        int x, y;
    };

    inline constexpr Vector operator-(const Pos& l, const Pos& r) { return {l.x - r.x, l.y - r.y}; }
    inline constexpr int cross(const Vector& u, const Vector& v) { return u.x * v.y - u.y * v.x; }
    inline constexpr int dot(const Vector& u, const Vector& v) { return u.x * v.x + u.y * v.y; }
    inline constexpr bool left(const Vector& u, const Vector& v) { return cross(u, v) > 0; }
    inline constexpr bool right(const Vector& u, const Vector& v) { return cross(u, v) < 0; }
    inline constexpr int sqr(int x) { return x * x; }
    inline float length(const Vector& v) { return sqrtf(float(sqr(v.x) + sqr(v.y))); }

    /// Normalized 2D vector.
    struct NormVector {
        NormVector() = default;
        NormVector(const NormVector&) = default;
        NormVector& operator=(const NormVector&) = default;
        explicit NormVector(const Vector& v) { operator=(v); }
        constexpr NormVector(float aX, float aY) : x(aX), y(aY) {}

        NormVector& operator=(const Vector& v) {
            float a = 1.f / length(v);
            x = a * v.x;
            y = a * v.y;
            return *this;
        }

        // data
        float x, y;
    };

    inline constexpr float dot(const NormVector& u, const NormVector& v) {
        return u.x * v.x + u.y * v.y;
    }
    inline constexpr NormVector perpendicular(const NormVector& v) {
        return NormVector{v.y, -v.x};
    }
}

#endif // FMO_ALGEBRA_HPP
