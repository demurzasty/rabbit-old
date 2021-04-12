#pragma once 

#include <cmath>

namespace rb {
    template<typename T>
    constexpr T pi() noexcept {
        return static_cast<T>(3.14159265358979323846264338327L);
    }

    template<typename T>
    constexpr T abs(const T value) noexcept {
        return value > 0 ? value : -value;
    }

    template<typename T>
    constexpr T sign(const T value) noexcept {
        return value > 0 ? static_cast<T>(1) : value < 0 ? static_cast<T>(-1) : static_cast<T>(0);
    }

    template<typename T>
    constexpr T min(const T a, const T b) noexcept {
        return a < b ? a : b;
    }

    template<typename T>
    constexpr T max(const T a, const T b) noexcept {
        return a > b ? a : b;
    }

    template<typename T>
    constexpr T clamp(const T value, const T a, const T b) noexcept {
        return value < a ? a : value > b ? b : value;
    }

#if defined FP_FAST_FMA
    template<typename T>
    constexpr T lerp(const T a, const T b, const T factor) noexcept {
        return std::fma(factor, b, std::fma(-factor, a, a));
    }
#else
    template<typename T>
    constexpr T lerp(const T a, const T b, const T factor) noexcept {
        return a * (T{ 1 } - factor) + b * factor;
    }
#endif
}
