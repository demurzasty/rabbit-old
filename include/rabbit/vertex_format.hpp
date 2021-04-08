#pragma once 

#include <cstdint>
#include <cstddef>

namespace rb {
    enum class vertex_format : std::uint8_t {
        vec2f,
        vec3f,
        vec4f,
        vec2i,
        vec3i,
        vec4i
    };

    [[nodiscard]] std::size_t vertex_format_size(const vertex_format format);
}
