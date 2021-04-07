#pragma once 

#include <cstdint>

namespace rb {
    enum class vertex_format : std::uint8_t {
        vec2f,
        vec3f,
        vec4f,
        vec2i,
        vec3i,
        vec4i
    };
}
