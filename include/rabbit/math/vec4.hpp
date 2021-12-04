#pragma once 

namespace rb {
    template<typename T>
    struct vec4 {
        static constexpr vec4<T> one() noexcept { return { 1, 1, 1, 1 }; }
        static constexpr vec4<T> zero() noexcept { return { 0, 0, 0, 0 }; }

        T x, y, z, w;
    };

    template<typename T>
    constexpr bool operator==(const vec4<T>& a, const vec4<T>& b) noexcept {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
    }

    template<typename T>
    constexpr bool operator!=(const vec4<T>& a, const vec4<T>& b) noexcept {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
    }

    template<typename T>
    constexpr vec4<T> operator+(const vec4<T>& a, const vec4<T>& b) noexcept {
        return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    }

    template<typename T>
    constexpr vec4<T> operator-(const vec4<T>& a, const vec4<T>& b) noexcept {
        return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
    }

    template<typename T>
    constexpr vec4<T> operator*(const vec4<T>& a, const vec4<T>& b) noexcept {
        return { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
    }

    template<typename T>
    constexpr vec4<T> operator/(const vec4<T>& a, const vec4<T>& b) noexcept {
        return { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
    }

    template<typename T>
    constexpr vec4<T> operator+(const vec4<T>& a, const T& b) noexcept {
        return { a.x + b, a.y + b, a.z + b, a.w + b };
    }

    template<typename T>
    constexpr vec4<T> operator-(const vec4<T>& a, const T& b) noexcept {
        return { a.x - b, a.y - b, a.z - b, a.w - b };
    }

    template<typename T>
    constexpr vec4<T> operator*(const vec4<T>& a, const T& b) noexcept {
        return { a.x * b, a.y * b, a.z * b, a.w * b };
    }

    template<typename T>
    constexpr vec4<T> operator/(const vec4<T>& a, const T& b) noexcept {
        return { a.x / b, a.y / b, a.z / b, a.w / b };
    }

    template<typename T>
    constexpr vec4<T> operator+(const T& a, const  vec4<T>& b) noexcept {
        return { a + b.x, a + b.y, a + b.z, a + b.w };
    }

    template<typename T>
    constexpr vec4<T> operator-(const T& a, const  vec4<T>& b) noexcept {
        return { a - b.x, a - b.y, a - b.z, a - b.w };
    }

    template<typename T>
    constexpr vec4<T> operator*(const T& a, const  vec4<T>& b) noexcept {
        return { a * b.x, a * b.y, a * b.z, a * b.w };
    }

    template<typename T>
    constexpr vec4<T> operator/(const T& a, const  vec4<T>& b) noexcept {
        return { a / b.x, a / b.y, a / b.z, a / b.w };
    }

    using vec4i = vec4<int>;
    using vec4u = vec4<unsigned int>;
    using vec4f = vec4<float>;
}
