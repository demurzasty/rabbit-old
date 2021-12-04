#pragma once 

#include <cmath>
#include <cstdint>

namespace rb {
    template<typename T>
    constexpr T pi() noexcept {
        return static_cast<T>(3.1415926535897931L);
    }

    template<typename T>
    constexpr T rad2deg(T rad) noexcept {
        return rad * 180 / pi<T>();
    }

    template<typename T>
    constexpr T deg2rad(T deg) noexcept {
        return deg * pi<T>() / 180;
    }

    template<typename T>
    constexpr bool is_power_of_two(T x) noexcept {
        return !(x == 0) && !(x & (x - 1));
    }

    template<typename T>
    constexpr T next_power_of_two(T v) noexcept {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        return ++v;
    }

    template<typename T>
    constexpr T min(const T& a, const T& b) noexcept {
        return (a < b) ? a : b;
    }

    template<typename T>
    constexpr T max(const T& a, const T& b) noexcept {
        return (a > b) ? a : b;
    }

    template<typename T>
    constexpr T clamp(const T& v, const T& a, const T& b) noexcept {
        return max(min(v, b), a);
    }

    template<typename T>
    constexpr T abs(T v) noexcept {
        return (v < 0) ? -v : v;
    }

    template<>
    inline float abs(float v) noexcept {
        (*reinterpret_cast<std::uint32_t*>(&v)) &= 0x7fffffff;
        return v;
    }

    template<typename T>
    constexpr T lerp(const T& a, const T& b, const T& t) noexcept {
        return a + (b - a) * t;
    }
}
