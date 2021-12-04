#pragma once 

#include <cstdint>

namespace rb {
    struct color {
        static constexpr color white() noexcept { return { 255, 255, 255, 255 }; }
        static constexpr color black() noexcept { return { 0, 0, 0, 255 }; }
        static constexpr color red() noexcept { return { 255, 0, 0, 255 }; }
        static constexpr color green() noexcept { return { 0, 255, 0, 255 }; }
        static constexpr color blue() noexcept { return { 0, 0, 255, 255 }; }
        static constexpr color transparent() noexcept { return { 0, 0, 0, 0 }; }

        std::uint8_t r, g, b, a;
    };
}