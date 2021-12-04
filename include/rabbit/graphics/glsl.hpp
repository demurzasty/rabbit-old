#pragma once 

#include "shader.hpp"
#include "../core/span.hpp"

#include <cstdint>
#include <vector>
#include <string>

namespace rb {
    class glsl {
    public:
        static std::vector<std::uint32_t> compile(shader_stage stage, const std::string& code, const span<const std::string> definitions = {});

        static std::vector<std::uint32_t> compile(shader_stage stage, const span<const std::uint8_t>& code, const span<const std::string> definitions = {});
    };
}