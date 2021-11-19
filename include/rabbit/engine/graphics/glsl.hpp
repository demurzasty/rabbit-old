#pragma once 

#include <rabbit/engine/core/span.hpp>
#include <rabbit/engine/graphics/shader.hpp>

#include <cstdint>
#include <vector>
#include <string>

// TODO: Use string_view for macro definitions?

namespace rb {
    /**
     * @brief GLSL helper class.
     */
    class glsl {
    public:
        static std::vector<std::uint32_t> compile(shader_stage stage, const std::string& code);

        static std::vector<std::uint32_t> compile(shader_stage stage, const std::string& code, const span<const std::string> definitions);
    };
}
