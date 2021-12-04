#pragma once 

namespace rb {
    template<typename T>
    struct vec2 {
        static constexpr vec2<T> one() noexcept { return { 1, 1 }; }
        static constexpr vec2<T> zero() noexcept { return { 0, 0 }; }

        T x, y;
    };

    template<typename T>
    constexpr bool operator==(const vec2<T>& a, const vec2<T>& b) noexcept {
        return a.x == b.x && a.y == b.y;
    }

    template<typename T>
    constexpr bool operator!=(const vec2<T>& a, const vec2<T>& b) noexcept {
        return a.x != b.x || a.y != b.y;
    }

    template<typename T>
    constexpr vec2<T> operator+(const vec2<T>& a, const vec2<T>& b) noexcept {
        return { a.x + b.x, a.y + b.y };
    }

    template<typename T>
    constexpr vec2<T> operator-(const vec2<T>& a, const vec2<T>& b) noexcept {
        return { a.x - b.x, a.y - b.y };
    }

    template<typename T>
    constexpr vec2<T> operator*(const vec2<T>& a, const vec2<T>& b) noexcept {
        return { a.x * b.x, a.y * b.y };
    }

    template<typename T>
    constexpr vec2<T> operator/(const vec2<T>& a, const vec2<T>& b) noexcept {
        return { a.x / b.x, a.y / b.y };
    }

    template<typename T>
    constexpr vec2<T> operator+(const vec2<T>& a, const T& b) noexcept {
        return { a.x + b, a.y + b };
    }

    template<typename T>
    constexpr vec2<T> operator-(const vec2<T>& a, const T& b) noexcept {
        return { a.x - b, a.y - b };
    }

    template<typename T>
    constexpr vec2<T> operator*(const vec2<T>& a, const T& b) noexcept {
        return { a.x * b, a.y * b };
    }

    template<typename T>
    constexpr vec2<T> operator/(const vec2<T>& a, const T& b) noexcept {
        return { a.x / b, a.y / b };
    }

    template<typename T>
    constexpr vec2<T> operator+(const T& a, const  vec2<T>& b) noexcept {
        return { a + b.x, a + b.y };
    }

    template<typename T>
    constexpr vec2<T> operator-(const T& a, const  vec2<T>& b) noexcept {
        return { a - b.x, a - b.y };
    }

    template<typename T>
    constexpr vec2<T> operator*(const T& a, const  vec2<T>& b) noexcept {
        return { a * b.x, a * b.y };
    }

    template<typename T>
    constexpr vec2<T> operator/(const T& a, const  vec2<T>& b) noexcept {
        return { a / b.x, a / b.y };
    }

    using vec2i = vec2<int>;
    using vec2u = vec2<unsigned int>;
    using vec2f = vec2<float>;
}
