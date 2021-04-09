#pragma once 

#include "math.hpp" // abs
#include "../core/json_fwd.hpp"

#include <cmath> // std::sqrt
#include <type_traits> // std::is_floating_point_v
#include <limits> // std::numeric_limits<...>::epsilon

namespace rb {
    template<typename T>
    struct vec3 {
        using scalar_type = T;

        static constexpr vec3<T> one() { return { 1, 1, 1 }; };
        static constexpr vec3<T> zero() { return { 0, 0, 0 }; };
        static constexpr vec3<T> forward() { return { 0, 0, 1 }; };
        static constexpr vec3<T> right() { return { 1, 0, 0 }; };
        static constexpr vec3<T> up() { return { 0, 1, 0 }; };

        T x, y, z;

        constexpr vec3() = default;

        constexpr vec3(T scalar)
            : x(scalar)
            , y(scalar)
            , z(scalar) {
        }

        constexpr vec3(T x, T y)
            : x(x)
            , y(y)
            , z(0) {
        }

        constexpr vec3(T x, T y, T z)
            : x(x)
            , y(y)
            , z(z) {
        }

        template<typename U>
        explicit constexpr vec3(const vec3<U>& vec) : x(static_cast<T>(vec.x)), y(static_cast<T>(vec.y)), z(static_cast<T>(vec.z)) {}

        constexpr vec3(const json& json)
            : x(json[0])
            , y(json[1])
            , z(json[2]) {
        }

        static T length(const vec3<T>& vec) {
            return std::sqrt(dot(vec, vec));
        }

        static vec3<T> normalize(const vec3<T>& vec) {
            const auto inv_length = 1 / length(vec);
            return { vec.x * inv_length, vec.y * inv_length, vec.z * inv_length };
        }

        static constexpr T dot(const vec3<T>& a, const vec3<T>& b) {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }
    };

    template<typename T>
    constexpr vec3<T> operator+(const vec3<T>& a, const vec3<T>& b) {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    template<typename T>
    constexpr vec3<T> operator+(const vec3<T>& a, T b) {
        return { a.x + b, a.y + b, a.z + b };
    }

    template<typename T>
    constexpr vec3<T> operator+(T a, const vec3<T>& b) {
        return { a + b.x, a + b.y, a + b.z };
    }

    template<typename T>
    constexpr vec3<T> operator-(const vec3<T>& a, const vec3<T>& b) {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    template<typename T>
    constexpr vec3<T> operator-(const vec3<T>& a, T b) {
        return { a.x - b, a.y - b, a.z - b };
    }

    template<typename T>
    constexpr vec3<T> operator-(T a, const vec3<T>& b) {
        return { a - b.x, a - b.y, a - b.z };
    }

    template<typename T>
    constexpr vec3<T> operator-(const vec3<T>& a) {
        return { -a.x, -a.y, -a.z };
    }

    template<typename T>
    constexpr vec3<T> operator*(const vec3<T>& a, const vec3<T>& b) {
        return { a.x * b.x, a.y * b.y, a.z * b.z };
    }

    template<typename T>
    constexpr vec3<T> operator*(const vec3<T>& a, T b) {
        return { a.x * b, a.y * b, a.z * b };
    }

    template<typename T>
    constexpr vec3<T> operator*(T a, const vec3<T>& b) {
        return { a * b.x, a * b.y, a * b.z };
    }

    template<typename T>
    constexpr vec3<T> operator/(const vec3<T>& a, const vec3<T>& b) {
        return { a.x / b.x, a.y / b.y, a.z / b.y };
    }

    template<typename T>
    constexpr vec3<T> operator/(const vec3<T>& a, T b) {
        return { a.x / b, a.y / b, a.z /  b };
    }

    template<typename T>
    constexpr vec3<T> operator/(T a, const vec3<T>& b) {
        return { a / b.x, a / b.y, a / a.z };
    }

    template<typename T>
    constexpr bool operator==(const vec3<T>& a, const vec3<T>& b) {
        if constexpr (std::is_floating_point_v<T>) {
            return abs(a.x - b.x) < std::numeric_limits<T>::epsilon() &&
                abs(a.y - b.y) < std::numeric_limits<T>::epsilon() &&
                abs(a.z - b.z) < std::numeric_limits<T>::epsilon();
        } else {
            return a.x == b.x && a.y == b.y && a.z == b.z;
        }
    }

    template<typename T>
    constexpr bool operator!=(const vec3<T>& a, const vec3<T>& b) {
        if constexpr (std::is_floating_point_v<T>) {
            return abs(a.x - b.x) > std::numeric_limits<T>::epsilon() ||
                abs(a.y - b.y) > std::numeric_limits<T>::epsilon() ||
                abs(a.z - b.z) > std::numeric_limits<T>::epsilon();
        } else {
            return a.x != b.x || a.y != b.y || a.z != b.z;
        }
    }

    using vec3i = vec3<int>;
    using vec3f = vec3<float>;
}
