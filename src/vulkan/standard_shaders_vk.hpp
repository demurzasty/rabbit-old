#pragma once 

#include <rabbit/span.hpp>

#include <cstdint>

namespace rb {
    class standard_shaders_vk {
    public:
        static const char* solid_pixel_shader();

        static const char* texture_pixel_shader();

        static const char* vertex_shader();
    };
}
