#pragma once 

#include "vec3.hpp"
#include "vec4.hpp"

#include <cstdint>
#include <type_traits> 

namespace rb {
    struct color {
        static constexpr color white() { return { 255, 255, 255, 255 }; };
        static constexpr color black() { return { 0, 0, 0, 255 }; };
        static constexpr color red() { return { 255, 0, 0, 255 }; };
        static constexpr color green() { return { 0, 255, 0, 255 }; };
        static constexpr color blue() { return { 0, 0, 255, 255 }; };
        static constexpr color magenta() { return { 255, 0, 255, 255 }; };
        static constexpr color cornflower_blue() { return { 100, 149, 237, 255 }; };

        color() = default;

        constexpr color(std::uint8_t r, std::uint8_t g, std::uint8_t b) 
            : r(r)
            , g(g)
            , b(b)
            , a(255) {
        }

        constexpr color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) 
            : r(r)
            , g(g)
            , b(b)
            , a(a) {
        }

        constexpr bool operator==(const color& color) const {
            return r == color.r && g == color.g && b == color.b && a == color.a;
        }

        constexpr bool operator!=(const color& color) const {
            return r != color.r || g != color.g || b != color.b || a != color.a;
        }

        constexpr std::uint32_t to_integer() const {
            return r | (g << 8) | (b << 16) | (a << 24);
        }

        template<typename T>
        constexpr vec3<T> to_vec3() const {
            if constexpr (std::is_floating_point_v<T>) {
                return { static_cast<T>(r) / 255.0f, static_cast<T>(g) / 255.0f, static_cast<T>(b) / 255.0f };
            } else {
                return { static_cast<T>(r), static_cast<T>(g), static_cast<T>(b) };
            }
        }

        template<typename T>
        constexpr vec4<T> to_vec4() const {
            if constexpr (std::is_floating_point_v<T>) {
                return { static_cast<T>(r) / 255.0f, static_cast<T>(g) / 255.0f, static_cast<T>(b) / 255.0f, static_cast<T>(a) / 255.0f };
            } else {
                return { static_cast<T>(r), static_cast<T>(g), static_cast<T>(b), static_cast<T>(a) };
            }
        }

        std::uint8_t r, g, b, a;
    };
}
