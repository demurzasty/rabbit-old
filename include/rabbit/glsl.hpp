#pragma once 

#include "shader.hpp"

#include <cstdint>
#include <vector>
#include <string>

namespace rb {
    /**
     * @brief GLSL helper class.
     */
    class glsl {
    public:
        static std::vector<std::uint32_t> compile(shader_stage stage, const std::string& code);
    };
}
