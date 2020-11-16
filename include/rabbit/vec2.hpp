#pragma once 

#include <rabbit/math.hpp> // abs
#include <rabbit/json.hpp>

#include <cmath> // std::sqrt, std::abs
#include <type_traits> // std::is_floating_point_v
#include <limits> // std::numeric_limits<...>::epsilon

namespace rb {
    template<typename T>
    struct vec2 {
        using scalar_type = T;

        static constexpr vec2<T> one() { return { 1, 1 }; };
        static constexpr vec2<T> zero() { return { 0, 0 }; };
        static constexpr vec2<T> up() { return { 0, 1 }; };

        T x, y;

        vec2() = default;

        constexpr vec2(T scalar)
            : x(scalar)
            , y(scalar) {
        }

        constexpr vec2(T x, T y)
            : x(x)
            , y(y) {
        }

        template<typename U>
        explicit constexpr vec2(const vec2<U>& vec)
            : x(static_cast<T>(vec.x))
            , y(static_cast<T>(vec.y)) {
        }

        constexpr vec2(const json& json)
            : x(json[0])
            , y(json[1]) {
        }

        static T length(const vec2<T>& vec) {
            return std::sqrt(dot(vec, vec));
        }

        static vec2<T> normalize(const vec2<T>& vec) {
            const auto inv_length = 1 / length(vec);
            return { vec.x * inv_length, vec.y * inv_length };
        }

        static constexpr T dot(const vec2<T>& a, const vec2<T>& b) {
            return a.x * b.x + a.y * b.y;
        }
    };

    template<typename T>
    constexpr vec2<T> operator+(const vec2<T>& a, const vec2<T>& b) {
        return { a.x + b.x, a.y + b.y };
    }

    template<typename T>
    constexpr vec2<T> operator+(const vec2<T>& a, T b) {
        return { a.x + b, a.y + b };
    }

    template<typename T>
    constexpr vec2<T> operator+(T a, const vec2<T>& b) {
        return { a + b.x, a + b.y };
    }

    template<typename T>
    constexpr vec2<T> operator-(const vec2<T>& a, const vec2<T>& b) {
        return { a.x - b.x, a.y - b.y };
    }

    template<typename T>
    constexpr vec2<T> operator-(const vec2<T>& a, T b) {
        return { a.x - b, a.y - b };
    }

    template<typename T>
    constexpr vec2<T> operator-(T a, const vec2<T>& b) {
        return { a - b.x, a - b.y };
    }

    template<typename T>
    constexpr vec2<T> operator-(const vec2<T>& a) {
        return { -a.x, -a.y };
    }

    template<typename T>
    constexpr vec2<T> operator*(const vec2<T>& a, const vec2<T>& b) {
        return { a.x * b.x, a.y * b.y };
    }

    template<typename T>
    constexpr vec2<T> operator*(const vec2<T>& a, T b) {
        return { a.x * b, a.y * b };
    }

    template<typename T>
    constexpr vec2<T> operator*(T a, const vec2<T>& b) {
        return { a * b.x, a * b.y };
    }

    template<typename T>
    constexpr vec2<T> operator/(const vec2<T>& a, const vec2<T>& b) {
        return { a.x / b.x, a.y / b.y };
    }

    template<typename T>
    constexpr vec2<T> operator/(const vec2<T>& a, T b) {
        return { a.x / b, a.y / b };
    }

    template<typename T>
    constexpr vec2<T> operator/(T a, const vec2<T>& b) {
        return { a / b.x, a / b.y };
    }

    template<typename T>
    constexpr bool operator==(const vec2<T>& a, const vec2<T>& b) {
        if constexpr (std::is_floating_point_v<T>) {
            return abs(a.x - b.x) < std::numeric_limits<T>::epsilon() &&
                abs(a.y - b.y) < std::numeric_limits<T>::epsilon();
        } else {
            return a.x == b.x && a.y == b.y;
        }
    }

    template<typename T>
    constexpr bool operator!=(const vec2<T>& a, const vec2<T>& b) {
        if constexpr (std::is_floating_point_v<T>) {
            return abs(a.x - b.x) > std::numeric_limits<T>::epsilon() ||
                abs(a.y - b.y) > std::numeric_limits<T>::epsilon();
        } else {
            return a.x != b.x || a.y != b.y;
        }
    }

    using vec2i = vec2<int>;
    using vec2f = vec2<float>;
}
