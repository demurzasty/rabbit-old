#pragma once 

#include "config.hpp"

#include <cmath>

namespace rb {
    template<typename T>
    RB_NODISCARD constexpr T pi() RB_NOEXCEPT {
        return static_cast<T>(3.14159265358979323846264338327L);
    }

    template<typename T>
    RB_NODISCARD constexpr T abs(const T value) RB_NOEXCEPT {
        return value > 0 ? value : -value;
    }

    template<typename T>
    RB_NODISCARD constexpr T sign(const T value) RB_NOEXCEPT {
        return value > 0 ? static_cast<T>(1) : value < 0 ? static_cast<T>(-1) : static_cast<T>(0);
    }

    template<typename T>
    RB_NODISCARD constexpr T min(const T a, const T b) RB_NOEXCEPT {
        return a < b ? a : b;
    }

    template<typename T>
    RB_NODISCARD constexpr T max(const T a, const T b) RB_NOEXCEPT {
        return a > b ? a : b;
    }

    template<typename T>
    RB_NODISCARD constexpr T clamp(const T value, const T a, const T b) RB_NOEXCEPT {
        return value < a ? a : value > b ? b : value;
    }

#if defined FP_FAST_FMA
    template<typename T>
    RB_NODISCARD constexpr T lerp(const T a, const T b, const T factor) RB_NOEXCEPT {
        return std::fma(factor, b, std::fma(-factor, a, a));
    }
#else
    template<typename T>
    RB_NODISCARD constexpr T lerp(const T a, const T b, const T factor) RB_NOEXCEPT {
        return a * (T{ 1 } - factor) + b * factor;
    }
#endif
}
