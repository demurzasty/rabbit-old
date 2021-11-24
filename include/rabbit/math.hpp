#pragma once 

#include <cmath>

namespace rb {
    template<typename T>
    constexpr T pi() noexcept {
        return T{ 3.1415926535897931L };
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
}
