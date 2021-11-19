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
}
