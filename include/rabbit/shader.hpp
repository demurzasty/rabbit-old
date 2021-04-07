#pragma once 

#include "span.hpp"
#include "buffer.hpp"
#include "vertex_desc.hpp"
#include <memory>
#include <cstdint>

// TODO: Geometry and Tesselation shaders.

namespace rb {
    struct shader_desc {
        span<const std::uint8_t> vertex_bytecode;
        span<const std::uint8_t> fragment_bytecode;
        vertex_desc vertex_desc = {
            { vertex_attribute::position, vertex_format::vec3f },
            { vertex_attribute::texcoord, vertex_format::vec2f },
            { vertex_attribute::normal, vertex_format::vec3f }
        };
    };

    struct shader {
        virtual ~shader() = default;
    };
}
