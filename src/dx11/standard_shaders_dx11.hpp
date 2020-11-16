#pragma once 

#include <rabbit/span.hpp>

#include <cstdint>

namespace rb {
    class standard_shaders_dx11 {
    public:
        static span<const std::uint8_t> solid_pixel_shader();

        static span<const std::uint8_t> texture_pixel_shader();

        static span<const std::uint8_t> vertex_shader();
    };
}
