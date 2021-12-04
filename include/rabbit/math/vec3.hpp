#pragma once 

#include "math.hpp"

namespace rb {
    template<typename T>
    struct vec3 {
        static constexpr vec3<T> one() noexcept { return { 1, 1, 1 }; }
        static constexpr vec3<T> zero() noexcept { return { 0, 0, 0 }; }

        T x, y, z;
    };

    template<typename T>
    constexpr bool operator==(const vec3<T>& a, const vec3<T>& b) noexcept {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }

    template<typename T>
    constexpr bool operator!=(const vec3<T>& a, const vec3<T>& b) noexcept {
        return a.x != b.x || a.y != b.y || a.z != b.z;
    }

    template<typename T>
    constexpr vec3<T> operator+(const vec3<T>& a, const vec3<T>& b) noexcept {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    template<typename T>
    constexpr vec3<T> operator-(const vec3<T>& a, const vec3<T>& b) noexcept {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    template<typename T>
    constexpr vec3<T> operator*(const vec3<T>& a, const vec3<T>& b) noexcept {
        return { a.x * b.x, a.y * b.y, a.z * b.z };
    }

    template<typename T>
    constexpr vec3<T> operator/(const vec3<T>& a, const vec3<T>& b) noexcept {
        return { a.x / b.x, a.y / b.y, a.z / b.z };
    }

    template<typename T>
    constexpr vec3<T> operator+(const vec3<T>& a, const T& b) noexcept {
        return { a.x + b, a.y + b, a.z + b };
    }

    template<typename T>
    constexpr vec3<T> operator-(const vec3<T>& a, const T& b) noexcept {
        return { a.x - b, a.y - b, a.z - b };
    }

    template<typename T>
    constexpr vec3<T> operator*(const vec3<T>& a, const T& b) noexcept {
        return { a.x * b, a.y * b, a.z * b };
    }

    template<typename T>
    constexpr vec3<T> operator/(const vec3<T>& a, const T& b) noexcept {
        return { a.x / b, a.y / b, a.z / b };
    }

    template<typename T>
    constexpr vec3<T> operator+(const T& a, const  vec3<T>& b) noexcept {
        return { a + b.x, a + b.y, a + b.z };
    }

    template<typename T>
    constexpr vec3<T> operator-(const T& a, const  vec3<T>& b) noexcept {
        return { a - b.x, a - b.y, a - b.z };
    }

    template<typename T>
    constexpr vec3<T> operator*(const T& a, const  vec3<T>& b) noexcept {
        return { a * b.x, a * b.y, a * b.z };
    }

    template<typename T>
    constexpr vec3<T> operator/(const T& a, const  vec3<T>& b) noexcept {
        return { a / b.x, a / b.y, a / b.z };
    }

    template<typename T>
    vec3<T> rotate(const vec3<T>& point, const vec3<T>& rotation) noexcept {
        const auto cosa = std::cos(rotation.y);
        const auto sina = std::sin(rotation.y);

        const auto cosb = std::cos(rotation.z);
        const auto sinb = std::sin(rotation.z);

        const auto cosc = std::cos(rotation.x);
        const auto sinc = std::sin(rotation.x);

        const auto axx = cosa * cosb;
        const auto axy = cosa * sinb * sinc - sina * cosc;
        const auto axz = cosa * sinb * cosc + sina * sinc;

        const auto ayx = sina * cosb;
        const auto ayy = sina * sinb * sinc + cosa * cosc;
        const auto ayz = sina * sinb * cosc - cosa * sinc;

        const auto azx = -sinb;
        const auto azy = cosb * sinc;
        const auto azz = cosb * cosc;

        return {
            axx * point.x + axy * point.y + axz * point.z,
            ayx * point.x + ayy * point.y + ayz * point.z,
            azx * point.x + azy * point.y + azz * point.z
        };
    }

    using vec3i = vec3<int>;
    using vec3u = vec3<unsigned int>;
    using vec3f = vec3<float>;
}
