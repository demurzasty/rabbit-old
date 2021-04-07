#pragma once 

#include "span.hpp"

#include <cstdint>

namespace rb {
    enum class builtin_shader : std::uint8_t {
        forward_vert,
        forward_frag
    };

    struct builtin_shaders {
        static span<const std::uint8_t> get(builtin_shader shader);
    };
}
