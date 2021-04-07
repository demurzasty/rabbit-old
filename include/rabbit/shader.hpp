#pragma once 

#include "span.hpp"

#include <cstdint>

// TODO: Geometry and Tesselation shaders.

namespace rb {
    struct shader_desc {
        span<const std::uint8_t> vertex_bytecode;
        span<const std::uint8_t> fragment_bytecode;
    };

    struct shader {
        virtual ~shader() = default;
    };
}
