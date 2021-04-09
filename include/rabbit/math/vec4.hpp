#pragma once 

#include "math.hpp" // abs
#include "vec2.hpp"
#include "../core/json_fwd.hpp"

#include <cmath> // std::sqrt
#include <type_traits> // std::is_floating_point_v
#include <limits> // std::numeric_limits<...>::epsilon

namespace rb {
    template<typename T>
    struct alignas(16) vec4 {
        using scalar_type = T;

        static constexpr vec4<T> one() { return { 1, 1, 1, 1 }; };
        static constexpr vec4<T> zero() { return { 0, 0, 0, 0 }; };

        T x, y, z, w;

        constexpr vec4() = default;

        constexpr vec4(T scalar)
            : x(scalar)
            , y(scalar)
            , z(scalar)
            , w(scalar) {
        }

        constexpr vec4(T x, T y, T z, T w)
            : x(x)
            , y(y)
            , z(z)
            , w(w) {
        }

        template<typename U>
        explicit constexpr vec4(const vec4<U>& vec)
            : x(static_cast<T>(vec.x))
            , y(static_cast<T>(vec.y))
            , z(static_cast<T>(vec.z))
            , w(static_cast<T>(vec.w)) {
        }

        constexpr vec4(const json& json)
            : x(json[0])
            , y(json[1])
            , z(json[2])
            , w(json[3]) {
        }

        constexpr bool contains(const vec2<T>& vec) const {
            return vec.x >= x && vec.x < x + z &&
                vec.y >= y && vec.y < y + w;
        }

        static T length(const vec4<T>& vec) {
            return std::sqrt(dot(vec, vec));
        }

        static vec4<T> normalize(const vec4<T>& vec) {
            const auto inv_length = 1 / length(vec);
            return { vec.x * inv_length, vec.y * inv_length, vec.z * inv_length, vec.w * inv_length };
        }

        static constexpr T dot(const vec4<T>& a, const vec4<T>& b) {
            return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        }
    };

    template<typename T>
    constexpr vec4<T> operator*(const vec4<T>& a, T b) {
        return { a.x * b, a.y * b, a.z * b, a.w * b };
    }

    template<typename T>
    constexpr bool operator==(const vec4<T>& a, const vec4<T>& b) {
        if constexpr (std::is_floating_point_v<T>) {
            return abs(a.x - b.x) < std::numeric_limits<T>::epsilon() &&
                abs(a.y - b.y) < std::numeric_limits<T>::epsilon() &&
                abs(a.z - b.z) < std::numeric_limits<T>::epsilon() &&
                abs(a.w - b.w) < std::numeric_limits<T>::epsilon();
        } else {
            return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
        }
    }

    template<typename T>
    constexpr bool operator!=(const vec4<T>& a, const vec4<T>& b) {
        if constexpr (std::is_floating_point_v<T>) {
            return abs(a.x - b.x) > std::numeric_limits<T>::epsilon() ||
                abs(a.y - b.y) > std::numeric_limits<T>::epsilon() ||
                abs(a.z - b.z) > std::numeric_limits<T>::epsilon() ||
                abs(a.w - b.w) > std::numeric_limits<T>::epsilon();
        } else {
            return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
        }
    }

    using vec4i = vec4<int>;
    using vec4f = vec4<float>;
}
