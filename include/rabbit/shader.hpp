#pragma once 

#include "span.hpp"
#include "buffer.hpp"

#include <memory>
#include <cstdint>

// TODO: Geometry and Tesselation shaders.

namespace rb {
    struct shader_desc {
        span<const std::uint8_t> vertex_bytecode;
        span<const std::uint8_t> fragment_bytecode;
    };

    struct shader {
        virtual ~shader() = default;

        virtual void bind_buffer(std::shared_ptr<buffer> uniform_buffer, std::size_t index) = 0;
    };
}
